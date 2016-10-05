
SRC = tcp_count.c
OBJ = $(SRC:%.c=%.so)

$(OBJ): $(SRC)
	gcc -shared -o $(OBJ) $(SRC) -I../zabbix-src/include/ -fPIC

clean:
	rm -f $(OBJ)

install:$(OBJ)
	service zabbix-agent stop
	install -C $(OBJ) /etc/zabbix/modules/
	service zabbix-agent start
	service zabbix-agent status
	tail /var/log/zabbix/zabbix_agentd.log

