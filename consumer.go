package max_monitor

import (
  log "github.com/Sirupsen/logrus"
  "menteslibres.net/gosexy/redis"
)

type Consumer struct {
  Host        string
  Port        uint
  Channel     string
}

func (c *Consumer) connect() *redis.Client {
  redisConnection := redis.New()
	err := redisConnection.ConnectNonBlock(c.Host, c.Port)
	if err != nil { log.Fatalf("Connection failed to connect: %s\n", err.Error()) }

  return redisConnection
}

func (c *Consumer) Latest() []string {
  redisConnection := c.connect()
  defer redisConnection.Quit()
  items, _ := redisConnection.LRange(c.Channel, 0, 9)

	return items
}

func (c *Consumer) Subscribe(notifications chan []string) {
	var msg []string
  rec := make(chan []string)

  redisConnection := c.connect()
  defer redisConnection.Quit()

	go redisConnection.Subscribe(rec, c.Channel)

	for {
		msg = <-rec
    log.Debug(msg)
		if msg[0] != "message" { continue }
		notifications <- msg
	}
}
