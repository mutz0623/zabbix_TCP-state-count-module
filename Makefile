
SRC = tcp_count.c
TARGET = $(SRC:%.c=%.so)
MODULEPATH = /etc/zabbix/modules

$(TARGET): $(SRC)
	gcc -shared -o $(TARGET) $(SRC) -I../zabbix-src/include/ -fPIC

clean:
	rm -f $(TARGET)

install:$(TARGET)
	service zabbix-agent stop
	install -C $(TARGET) $(MODULEPATH)
	service zabbix-agent start
	service zabbix-agent status
	tail /var/log/zabbix/zabbix_agentd.log

test:
	md5sum  $(MODULEPATH)/$(TARGET) ./$(TARGET)
	zabbix_get -s 127.0.0.1 -k net.tcp.count[]
	ss -a -t -n |tail -n+2 |wc -l
	zabbix_get -s 127.0.0.1 -k net.tcp.count[10050]
	ss sport = :10050 -a -t -n |tail -n+2 |wc -l
	zabbix_get -s 127.0.0.1 -k net.tcp.count[,10050]
	ss dport = :10050 -a -t -n |tail -n+2 |wc -l
	zabbix_get -s 127.0.0.1 -k net.tcp.count[,,LISTEN]
	ss state listening -a -t -n |tail -n+2 |wc -l

