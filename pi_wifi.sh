#!/bin/bash
mode=$1
shift

function usage () {
	echo "$0 (ap|router|sta|-i) configfile"
	echo "   configile effctive only in option -i set"
}

case "$mode" in
	router)
		echo "Router start .... "
		#/etc/init.d/hostapd stop
		killall -9 hostapd
		ip l s dev wlan0 nomaster
		echo 1 > /proc/sys/net/ipv4/ip_forward
		iptables -t nat -A POSTROUTING -o wlan0 -j MASQUERADE
		systemctl restart wpa_supplicant
		/etc/init.d/udhcpd restart
		#udhcpd
		;;
	ap)
		echo "Ap start .... "
		systemctl stop wpa_supplicant
		#/etc/init.d/hostapd restart
		killall -9 hostapd
		hostapd -B /etc/hostapd/hostapd.conf
		/etc/init.d/udhcpd restart
		#/etc/default/udhcpd set enable
		#udhcpd
		;;
	
	sta)
		echo "Station start .... "
		#/etc/init.d/hostapd stop
		killall -9 hostapd
		ip l s dev wlan0 nomaster
		ip a flush dev wlan0
		echo 0 > /proc/sys/net/ipv4/ip_forward
		systemctl restart wpa_supplicant
		;;
	-i)
		if [ ! -e "$1" ] ; then
			echo "Not find config $1"
			exit 1
		fi
		$0 `cat $1`
		;;
	*)
		usage $0
		;;
esac

echo "---------------" >> /root/a.log

exit 0
