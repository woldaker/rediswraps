return {
  redis.call('ECHO', 'This'),
  redis.call('ECHO', 'command'),
  redis.call('ECHO', 'is'),
  redis.call('ECHO', 'pointless'),
  redis.call('ECHO', '!')
}

