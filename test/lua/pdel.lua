local keys = redis.pcall( 'KEYS', ARGV[ 1 ] )

if #keys == 0 then
  return 0
end

return redis.pcall( 'DEL', unpack( keys ) )
