# gb28181_client

国标 gb28181 模拟客户端

- 支持 Linux
- 支持 MacOS

## 编译

```
cd gb28281_client
./init.sh

mkdir build && cd build
cmake ..
make
```

## 运行
```
./gb28181-client --server-ip 10.200.20.26 --server-id 34020000002000000001 --server-port 5064 --device-id 31011500991320000046 --filepath xxx.264
./gb28181-client --server-ip 127.0.0.1 --server-id 43000000802008000001 --server-port 5061 --device-id 43000000801320008064 --device-port 15061 --filepath /home/alex/programs/gb28181_client/samples/bigbuckbunny_1280x720_24fps_annexb_012.h264

```