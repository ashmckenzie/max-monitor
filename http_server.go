package max_monitor

import (
  "encoding/json"
  "fmt"
  "html/template"
  "net/http"
  "strings"
  "strconv"
  log "github.com/Sirupsen/logrus"
  // "github.com/davecgh/go-spew/spew"
)

var chttp = http.NewServeMux()
var templates = template.Must(template.ParseFiles("index.tmpl.html"))

type HTTPServer struct {
  Host           string
	Consumer       *Consumer
  msgBroker      *Broker
  notifications  chan []string
}

type Notification struct {
  GraphType   string    `json:"graph_type"`
  Value       float64   `json:"value"`
  Epoch       int64     `json:"epoch"`
}

func (n *Notification) asJSON() string {
  data, _ := json.Marshal(n)
  return string(data)
}

type Broker struct {
	subscribers map[chan *Notification]bool
}

func (b *Broker) Subscribe() chan *Notification {
	ch := make(chan *Notification)
	b.subscribers[ch] = true
	return ch
}

func (b *Broker) Unsubscribe(ch chan *Notification) {
	delete(b.subscribers, ch)
}

func (b *Broker) Publish(channel string, epoch int64, value float64) {
	for ch := range b.subscribers {
    notification := Notification{ channel, value, epoch }
		ch <- &notification
	}
}

func NewBroker() *Broker {
	return &Broker{make(map[chan *Notification]bool)}
}

func (h *HTTPServer) Listen() {
	h.msgBroker = NewBroker()
  h.notifications = make(chan []string)

  log.Infof("HTTP listening at %s", h.Host)

  go h.Consumer.Subscribe(h.notifications)
  go h.consumeNotifications()

  chttp.Handle("/", http.FileServer(http.Dir("./public")))
  http.HandleFunc("/", h.httpHandler)
  http.ListenAndServe(h.Host, nil)
}

func (h *HTTPServer) httpHandler(w http.ResponseWriter, r *http.Request) {
  log.Printf("%s", r.URL.Path)

  switch r.URL.Path {
    case "/":             h.actionIndex(w, r)
    case "/latest/json":  h.actionLatestJSON(w, r)
    case "/stream/sse":   h.actionStreamServerSentEvents(w, r)
    case "/stream/json":  h.actionStreamJSON(w, r)
    default:              h.actionTryStatic(w, r)
  }
}

func (h *HTTPServer) actionIndex(w http.ResponseWriter, r *http.Request) {
  templates.ExecuteTemplate(w, "index.tmpl.html", nil)
}

func (h *HTTPServer) actionStreamServerSentEvents(w http.ResponseWriter, r *http.Request) {
  notifications, quit := h.setupNotifications(w, r)

  for {
		notification := <-notifications
    _, err := fmt.Fprintf(w, "event: notifications\ndata: %s\n\n", notification.asJSON())
    if err != nil {
      quit <-true
      break
    }
	}
}

func (h *HTTPServer) getLatest(done chan bool, messages chan Notification) {
  var item string
  items := h.Consumer.Latest()

  for i := len(items)-1; i >= 0; i-- {
    item = items[i]

    event_epoch_and_value := strings.Split(item, ",")

    if len(event_epoch_and_value) != 3 { continue }

    event := event_epoch_and_value[0]
    epoch, _ := strconv.ParseInt(event_epoch_and_value[1], 10, 0)
    value, _ := strconv.ParseFloat(event_epoch_and_value[2], 64)

    notification := Notification{ event, value, epoch }

    messages <- notification
  }

  done <- true
}

func (h *HTTPServer) actionStreamJSON(w http.ResponseWriter, r *http.Request) {
  notifications, quit := h.setupNotifications(w, r)

  for {
		notification := <-notifications
    _, err := fmt.Fprintf(w, "%s\n", notification.asJSON())
    if err != nil {
      quit <-true
      break
    }
	}
}

func (h *HTTPServer) actionLatestJSON(w http.ResponseWriter, r *http.Request) {
  var notifications []Notification

  done := make(chan bool)
  messages := make(chan Notification)

  go h.getLatest(done, messages)

  go func() {
    for {
      notification := <- messages
      notifications = append(notifications, notification)
    }
  }()

  <- done

  data, _ := json.Marshal(notifications)
  fmt.Fprintf(w, "%s\n", data)
}

func (h *HTTPServer) actionStream(w http.ResponseWriter, r *http.Request, n chan<- *Notification, q chan bool) {
	f, _ := w.(http.Flusher)

  w.Header().Set("Content-Type", "text/event-stream")
  w.Header().Set("Cache-Control", "no-cache")
  w.Header().Set("Connection", "keep-alive")

  quit := false
  ch := h.msgBroker.Subscribe()
	defer h.msgBroker.Unsubscribe(ch)

	for {
    if quit == true { break }

    select {
		case notification := <-ch:
      n <-notification
      f.Flush()
    case quit = <-q:
      log.Debug("Disconnecting client")
      break
    }
	}
}

func (h *HTTPServer) actionTryStatic(w http.ResponseWriter, r *http.Request) {
  chttp.ServeHTTP(w, r)
}

func (h *HTTPServer) setupNotifications(w http.ResponseWriter, r *http.Request) (chan *Notification, chan bool) {
  notifications := make(chan *Notification)
  quit := make(chan bool)
  go h.actionStream(w, r, notifications, quit)

  return notifications, quit
}

func (h *HTTPServer) consumeNotifications() {
  var msg []string

  for {
    msg = <- h.notifications

    // message rpi-moteino-collector:max-room temperature,1451877565,23.50
    event_epoch_and_value := strings.Split(msg[2], ",")
    event := event_epoch_and_value[0]
    epoch, _ := strconv.ParseInt(event_epoch_and_value[1], 10, 0)
    value, _ := strconv.ParseFloat(event_epoch_and_value[2], 64)

    h.msgBroker.Publish(event, epoch, value)
  }
}
