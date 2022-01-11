# RCOM Presentation Config
## IP Config
- Configure gnu53 and gnu54 to have an IP address and have the network 172.16.50.0/24 defined and gnu52 to have the network 172.16.51.1/24 
- `ifconfig eth0 172.16.51.1/24` (gnu52)
- `ifconfig eth0 172.16.50.1/24` (gnu53)
- `ifconfig eth0 172.16.50.254/24` (gnu54)
- `ifconfig eth1 172.16.51.253/24` (gnu54)

## VLAN Config
### Resetting the Switch
- `enable`
- `configure terminal`
- `no vlan 2-4094`
- `exit`
- `copy flash:gnu5-clean startup-config`
- `reload`

### Create and Config VLANs 50 and 51
- `enable`
- `configure terminal`
- Password is `8nortel`
- `vlan 50` create vlan
- `vlan 51` 
- `interface fa 0/1` add port 1 to vlan 50 (gnu53 eth0)
- `switchport mode access`
- `switchport access vlan 50`
- `interface fa 0/2` add port 2 to vlan 50 (gnu54 eth0)
- `switchport mode access`
- `switchport access vlan 50`
- `interface fa 0/13` add port 13 to vlan 51 (gnu52 eth0)
- `switchport mode access`
- `switchport access vlan 51`
- `interface fa 0/14` add port 14 to vlan 51 (gnu54 with eth1)
- `switchport mode access`
- `switchport access vlan 51`
- `end`

### See VLAN info
- `show vlan id 50`
- `show vlan id 51`
- `show vlan brief`

- `show running-config interface fa 0/1`
- `show interfaces fa 0/1 switchport`

## Save Configs Switch/Router
- `copy running-config flash:<filename>`
- Using filename `t7g7`

## Config from file
- `copy flash:<filename> running-config`

## Linux Routing
### In gnu54:
- Enabling forwarding in gnus



- `echo 1 > /proc/sys/net/ipv4/ip_forward`
- Enabling echo reply to broadcast request
- `echo 0 > /proc/sys/net/ipv4/icmp_echo_ignore_broadcasts`

### Create route 2->3 and 3->2 via 4
- `ip route add 172.16.51.0/24 via 172.16.50.254` in gnu53
- `ip route add 172.16.50.0/24 via 172.16.51.253` in gnu52

## Cisco Routing
- User = `root`
- Pass = `8nortel` 
- `conf t`
- `interface fa 0/0`
- `ip address 172.16.51.254 255.255.255.0`
- `no shutdown`
- `ip nat inside`
- `exit`
- `interface fa 0/1`
- `ip address 172.16.2.59 255.255.255.0`
- `no shutdown`
- `ip nat outside`
- `exit`
- `ip nat pool ovrld 172.16.2.59 172.16.2.59 prefix 24`
- `ip nat inside source list 1 pool ovrld overload`
- `access-list 1 permit 172.16.50.0 0.0.0.7`
- `access-list 1 permit 172.16.51.0 0.0.0.7`
- `ip route 0.0.0.0 0.0.0.0 172.16.2.254`
- `ip route 172.16.50.0 255.255.255.0 172.16.51.253`
- `end`
- `ip route add default via 172.16.51.254` in gnu52 and gnu54
- `ip route add default via 172.16.50.254` in gnu53

## Notes
- Just `ifconfig` shows the IP info.
- Just `route` to see the routes.
- Optional `ping` to verify connection. Ping broadcast is `ping -b`.
- `systemctl restart networking` to reset configs.
- Using (W=2) and (Y=5).


- Configuration file for the Cisco Router 
 ```
Cisco NAT
http://www.cisco.com/en/US/tech/tk648/tk361/technologies_tech_note09186a0080094e77.shtml
conf t
interface gigabitethernet 0/0 *
ip address 172.16.y1.254 255.255.255.0
no shutdown
ip nat inside
exit
interface gigabitethernet 0/1*
ip address 172.16.1.y9 255.255.255.0
no shutdown
ip nat outside
exit
ip nat pool ovrld 172.16.1.y9 172.16.1.y9 prefix 24
ip nat inside source list 1 pool ovrld overload
access-list 1 permit 172.16.y0.0 0.0.0.7
access-list 1 permit 172.16.y1.0 0.0.0.7
ip route 0.0.0.0 0.0.0.0 172.16.1.254
ip route 172.16.y0.0 255.255.255.0 172.16.y1.253
end
* In room I320 use interface fastethernet
```