obj="pthread_lock_linux"
src="pthread_lock_linux.c"

# Compile
gcc -o $obj $src -lpthread

# Run
if [ -f $obj ]; then
    ./$obj
fi

# Clean
rm -f $obj