On your Linux:

If you want to run inside container then:

docker run -it \

  --cap-add=NET_ADMIN \

  -e DISPLAY=$DISPLAY \

  -e WAYLAND_DISPLAY=$WAYLAND_DISPLAY \

  -e XDG_RUNTIME_DIR=$XDG_RUNTIME_DIR \

  -v /tmp/.X11-unix:/tmp/.X11-unix \

  -v /mnt/wslg:/mnt/wslg \

  IMAGE_ID \

  bash


The 'IMAGE_ID' is your docker image, ex. amd64/gcc:latest

I am assuming you are now entering the container bash terminal.

If you directly use Linux or Virtualized Linux (ex. WSL2 Linux) then you can ignore above steps and only do following:

git clone https://github.com/neoteamer5/epiroc

cd epiroc

chmod +x SetupLinux.sh StartPLC.sh

Step 1 (in current linux terminal):

./SetupLinux.sh

Step 2 (open a new linux terminal):

./StartPLC.sh
