package main

import (
  "os"
  "strconv"
  "strings"
  log "github.com/Sirupsen/logrus"
  "github.com/codegangsta/cli"
  "github.com/ashmckenzie/max-monitor"
)

func main() {
  app := cli.NewApp()
  app.Name = "max-monitor"
  app.Usage = "Max Monitor"
  app.Version = "0.1.0"

  app.Flags = []cli.Flag {
    cli.StringFlag{
      Name: "http-listen",
      Usage: "HTTP host:port to listen on",
      Value: "0.0.0.0:9999",
      EnvVar: "HTTP_LISTEN",
    },
    cli.StringFlag{
      Name: "redis-host",
      Usage: "redis host:port to consume from",
      Value: "127.0.0.1:6379",
      EnvVar: "REDIS_HOST",
    },
    cli.StringFlag{
      Name: "redis-channel",
      Usage: "redis channel to subscribe to",
      Value: "rpi-moteino-collector:max-room",
      EnvVar: "REDIS_CHANNEL",
    },
  }

  app.Action = func(c *cli.Context) {
    setupLogging()
    subscribeAndListen(c)
  }

  app.Run(os.Args)
}

func setupLogging() {
  logLevel := log.InfoLevel
  if os.Getenv("DEBUG") == "true" { logLevel = log.DebugLevel }
  log.SetLevel(logLevel)
}

func subscribeAndListen(c *cli.Context) {
  redisChannel := c.String("redis-channel")
  redisHost, redisPort := extractRedisConnectionDetails(c.String("redis-host"))

  consumer   := max_monitor.Consumer{ Host: redisHost, Port: redisPort, Channel: redisChannel }
  httpServer := max_monitor.HTTPServer{ Host: c.String("http-listen"), Consumer: &consumer }

  httpServer.Listen()
}

func extractRedisConnectionDetails(str string) (string, uint) {
  r := strings.Split(str, ":")
  redisHost := r[0]
  redisPort, _ := strconv.ParseUint(r[1], 10, 64)

  return redisHost, uint(redisPort)
}
