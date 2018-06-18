mkdir -p build/linux

echo "building bmp..."  
gcc -c -g -MMD -MP -MF "build/linux/byte_msg_parser.o.d" -o build/linux/byte_msg_parser.o byte_msg_parser.c

echo "building dbc.."
gcc -c -g -MMD -MP -MF "build/linux/db_connection.o.d" -o build/linux/db_connection.o db_connection.c `mysql_config --cflags --libs`

echo "building h..."
gcc -c -g -MMD -MP -MF "build/linux/hashtable.o.d" -o build/linux/hashtable.o ./hashtable/hashtable.c

echo "building h_itr.."
gcc -c -g -MMD -MP -MF "build/linux/hashtable_itr.o.d" -o build/linux/hashtable_itr.o ./hashtable/hashtable_itr.c

echo "building h_u..."
gcc -c -g -MMD -MP -MF "build/linux/hashtable_utility.o.d" -o build/linux/hashtable_utility.o ./hashtable/hashtable_utility.c

echo "building dcm..."
gcc -c -g -MMD -MP -MF "build/linux/distribution_container_manager.o.d" -o build/linux/distribution_container_manager.o distribution_container_manager.c

echo "building m..."
gcc -c -g -MMD -MP -MF "build/linux/main.o.d" -o build/linux/main.o main.c

mkdir -p dist/linux

echo "building executable...\n\n"
gcc -o dist/linux/dcr_server -lm build/linux/byte_msg_parser.o build/linux/db_connection.o build/linux/hashtable.o build/linux/hashtable_itr.o build/linux/hashtable_utility.o build/linux/distribution_container_manager.o build/linux/main.o -lpthread `mysql_config --cflags --libs`
