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
./gb28181-client --server-ip 127.0.0.1 --server-id 43000000802008000001 --server-port 5061 --device-id 43000000801320008064 --device-port 15061 --filepath /root/programs/gb28181_client/samples/bigbuckbunny_1280x720_24fps_annexb_012.h264


测试服务器：
http://222.77.87.138:8081/#/login
admin
admin

serverip = "222.77.87.138"
serverid = "36012200002000000001"
userid = "36012200002021080403"
userpwd = "12345678"
serverport = 8083
channelId = "43000000801310008065"
```