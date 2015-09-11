var graph;
var graphs = {};
var graphData = {}

var temperatureRange = [ 15, 30 ];
var humidityRange = [ 40, 60 ];

function getData() {
  $.getJSON('/latest/json', function(data) {
    $.each(data, function(key, val) { addToGraphData(val) });
    setupGraphs();
    subscribe();
  });
}

function addToGraphData(val) {
  graph_type = val.graph_type
  delete(val.graph_type)
  if (!graphData[graph_type]) { graphData[graph_type] = [] }
  value = { time: val.epoch, y: val.value }
  graphData[graph_type].push(value)
}

function setupGraphs() {
  graphs['temperature'] = $('.temperature-history').epoch({
    type:    'time.line',
    data:    [ { label: "Temperature", values: graphData.temperature, range: temperatureRange } ],
    axes:    [ 'left', 'right', 'bottom' ],
    range:   { left: temperatureRange },
    margins: { top: 30, right: 30, bottom: 30, left: 30 }
  });

  graphs['humidity'] = $('.humidity-history').epoch({
    type:    'time.line',
    data:    [ { label: "Humidity", values: graphData.humidity, range: humidityRange } ],
    axes:    [ 'left', 'right', 'bottom' ],
    range:   { left: humidityRange },
    margins: { top: 30, right: 30, bottom: 30, left: 30 }
  });
}

function now() {
  return (new Date).getTime() / 1000;
}

function subscribe() {
  source = new EventSource('/stream/sse');
  source.addEventListener('notifications', function(notification) {
    processData(JSON.parse(notification.data));
  }, false);
}

function processData(data) {
  $('.' + data.graph_type + '-value').text(data.value).change();
  value = [ { time: data.epoch, y: data.value } ];
  graphs[data.graph_type].push(value)
  $('.updated-value').text(moment().format("YYYY-MM-DD HH:mm:ss ZZ"));
}

$(function() {
  getData();
});
