# About

zabbix loadable module for aggregating TCP sessions.

# Requirements

- zabbix 3.0 or later
- Linux

# How to use

Below is example for zabbix 3.0.5; but now already released newer version.
Please replace correct version for your environment.

downlaod and extract zabbix source code
```
$ wget  http://downloads.sourceforge.net/project/zabbix/ZABBIX%20Latest%20Stable/3.0.5/zabbix-3.0.5.tar.gz
$ tar zxf zabbix-3.0.5.tar.gz
```

create symlink for zabbix source dir
```
$ ln -s zabbix-3.0.5 zabbix-src
```

execute cofigrure
(When ./configure, almost envirement would have to set --enable-ipv6 option)
```
$ cd zabbix-src
$ ./configure --enable-ipv6
$ cd -
```

download or git clone the code of  this module, and make this.
```
$ git  clone https://github.com/mutz0623/zabbix_TCP-state-count-module.git
$ cd zabbix_TCP-state-count-module
$ make
```
modify zabbix_agentd.conf
 (LoadableModulePath,LoadableModule)
copy module to setting path
```
$ vi /etc/zabbix/zabbix_agentd.conf
$ cp tcp_count.so /etc/zabbix/module/
```

restart zabbix-agent service
```
$ sudo systemctl restart zabbix-agent.service
```
or
```
$ sudo service zabbix-agent restart
```

set item key "net.tcp.count[\<src port\>,\<dest port\>,\<state\>]" in your zabbix server

## misc

- In situation enable SELinux, need additional SELinux rule.  
for example below.

```
cat <<EOT >zabbix-TCPcount-rule.te
module zabbix-TCPcount-rule 1.0;

require {
        type zabbix_agent_t;
        class netlink_tcpdiag_socket { create nlmsg_read read write };
}

#============= zabbix_agent_t ==============
allow zabbix_agent_t self:netlink_tcpdiag_socket { read write };
allow zabbix_agent_t self:netlink_tcpdiag_socket { create nlmsg_read };
EOT

make -f /usr/share/selinux/devel/Makefile zabbix-TCPcount-rule.pp
semodule -i zabbix-TCPcount-rule.pp
```

