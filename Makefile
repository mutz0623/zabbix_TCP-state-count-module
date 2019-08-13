
CC = gcc
CFLAG = -Wall -Wextra -O3
SRC = tcp_count.c tcp_count_netlink.c
HDR = tcp_count_netlink.h
OBJ = $(SRC:%.c=%.o)
TARGET = tcp_count.so
MODULEPATH = /etc/zabbix/modules

.SUFFIXES: .c .o

$(TARGET): $(OBJ)
	$(CC) -shared -o $@ $(OBJ) -fPIC

$(OBJ): $(HDR)

.c.o:
	$(CC) -c $< -I../zabbix-src/include/ -fPIC -c $(CFLAG)

clean:
	rm -f $(TARGET) $(OBJ)

install:$(TARGET)
	service zabbix-agent stop
	install -d $(MODULEPATH)
	install -C $(TARGET) $(MODULEPATH)
	service zabbix-agent start
	service zabbix-agent status
	tail /var/log/zabbix/zabbix_agentd.log

test:
	md5sum  $(MODULEPATH)/$(TARGET) ./$(TARGET) || :
	zabbix_get -s 127.0.0.1 -k net.tcp.count[]
	ss -a -t -n |tail -n+2 |wc -l
	zabbix_get -s 127.0.0.1 -k net.tcp.count[10050]
	ss sport = :10050 -a -t -n |tail -n+2 |wc -l
	zabbix_get -s 127.0.0.1 -k net.tcp.count[,10050]
	ss dport = :10050 -a -t -n |tail -n+2 |wc -l
	zabbix_get -s 127.0.0.1 -k net.tcp.count[,,LISTEN]
	ss state listening -a -t -n |tail -n+2 |wc -l
	zabbix_get -s 127.0.0.1 -k net.tcp.count.bulk |jq .


test2:
	ss sport = :10051 -a -t -n |awk 'NR>1{count[$$1]++} END{for(key in count){print key,count[key]} }'
	zabbix_get -s 127.0.0.1 -k net.tcp.count.bulk[10051] |jq .
	ss dport = :10050 -a -t -n |awk 'NR>1{count[$$1]++} END{for(key in count){print key,count[key]} }'
	zabbix_get -s 127.0.0.1 -k net.tcp.count.bulk[,10050] |jq .


