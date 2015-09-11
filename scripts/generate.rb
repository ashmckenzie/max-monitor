#!/usr/bin/env ruby

require 'redis'

SLEEP = 1

redis_connection = Redis.new
channel = 'rpi-moteino-collector:max-room'

loop do
  events = [
    { name: 'temperature', value: sprintf('%.2f', rand(7.0) + 18.0) },
    { name: 'humidity', value: sprintf('%.2f', rand(7.0) + 45.0) }
  ]

  events.each do |event_name_and_value|
    data = '%s,%s,%.2f' % [ event_name_and_value[:name], Time.now.to_i, event_name_and_value[:value] ]

    redis_connection.lpush(channel, data)
    redis_connection.ltrim(channel, 0, 10)
    redis_connection.publish(channel, data)
  end

  print '.'
  sleep(SLEEP)
end
