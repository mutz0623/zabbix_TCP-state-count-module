CC = gcc
CFLAG = -MMD -MP -Wall -Wextra -O3 -fPIC
LDFLAG = -shared
INCLUDE = -I../zabbix-src/include
SRCDIR = ./src
SRC = $(wildcard $(SRCDIR)/*.c)
OBJDIR = ./obj
OBJ = $(addprefix $(OBJDIR)/, $(notdir $(SRC:.c=.o)))
BINDIR = ./bin
BIN = tcp_count.so
TARGET = $(BINDIR)/$(BIN)
DEPENDS = $(OBJ:.o=.d)
MODULEPATH = /etc/zabbix/modules

$(TARGET): $(OBJ)
	-mkdir -p $(BINDIR)
	$(CC) $(LDFLAG) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	-mkdir -p $(OBJDIR)
	$(CC) $(CFLAG) $(INCLUDE) -o $@ -c $<

-include $(DEPENDS)

.PHONY: all clean
all: clean $(TARGET)

clean:
	-rm -f $(TARGET) $(OBJ) $(DEPENDS)

.PHONY: install test test2
install:$(TARGET)
	service zabbix-agent stop
	install -d $(MODULEPATH)
	install -C $(TARGET) $(MODULEPATH)
	service zabbix-agent start
	service zabbix-agent status
	tail /var/log/zabbix/zabbix_agentd.log

test:
	md5sum  $(MODULEPATH)/$(BIN) ./$(TARGET) || :
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

.PHONY :doc doc-clean
doc:
	doxygen ./doc/Doxyfile

doc-clean:
	-rm -rf ./doc/html/

.PHONY : scan-build scan-build-clean
scan-build:
	$(MAKE) clean
	-mkdir -p ./doc/scan-build/
	scan-build -o ./doc/scan-build/ $(MAKE)

scan-build-clean:
	-rm -rf ./doc/scan-build/*

.PHONY: clean-all
clean-all: clean doc-clean scan-build-clean
	$(MAKE) clean
	$(MAKE) doc-clean
	$(MAKE) scan-build-clean

