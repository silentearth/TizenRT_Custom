/****************************************************************************
 *
 * Copyright 2016 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * net/netdev/netdev_ioctl.c
 *
 *   Copyright (C) 2007-2012, 2015 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>
#if defined(CONFIG_NET) && CONFIG_NSOCKET_DESCRIPTORS > 0

#include <sys/socket.h>
#include <sys/ioctl.h>

#include <string.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include <netdb.h>

#include <tinyara/net/net.h>
#include <tinyara/net/ip.h>
#include <tinyara/kmalloc.h>

#include <net/if.h>
#include <net/route.h>
#include <net/ethernet.h>
#include <netinet/in.h>

#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "lwip/igmp.h"
#include "lwip/dhcp.h"
#include "lwip/mld6.h"
#include "lwip/ip6.h"
#include "lwip/stats.h"

#ifdef CONFIG_NET_IGMP
#include "sys/sockio.h"
#endif							/* CONFIG_NET_IGMP */

#include "socket/socket.h"
#include "netdev/netdev.h"

#if LWIP_DNS
extern void dns_setserver(u8_t numdns, const ip_addr_t * dnsserver);
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NET_DEV_MTU(d)  ((d)->mtu)

/* This is really kind of bogus.. When asked for an IP address, this is
 * family that is returned in the ifr structure.  Probably could just skip
 * this since the address family has nothing to do with the Ethernet address.
 */

#ifdef CONFIG_NET_IPv6
#define AF_INETX AF_INET6
#else							/* CONFIG_NET_IPv6 */
#define AF_INETX AF_INET
#endif							/* CONFIG_NET_IPv6 */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void netdev_soft_ifup(FAR struct netif *dev)
{
	err_t res = netifapi_netif_set_up(dev);
	if (res != ERR_OK) {
		ndbg("netdev soft ifup fail\n");
	}
}

static void netdev_soft_ifdown(FAR struct netif *dev)
{
	err_t res = netifapi_netif_set_down(dev);
	if (res != ERR_OK) {
		ndbg("netdev soft ifdown fail\n");
	}
}

/****************************************************************************
 * Name: ioctl_addipv4route
 *
 * Description:
 *   Add an IPv4 route to the routing table.
 *
 * Input Parameters:
 *   rentry - Describes the route to be added
 *
 ****************************************************************************/

#if defined(CONFIG_NET_ROUTE) && defined(CONFIG_NET_IPv4)
static int ioctl_addipv4route(FAR struct rtentry *rtentry)
{
	FAR struct sockaddr_in *addr;
	in_addr_t target;
	in_addr_t netmask;
	in_addr_t router;

	addr = (FAR struct sockaddr_in *)rtentry->rt_target;
#if CONFIG_NET_LWIP
	target = (in_addr_t)ip_2_ip4(addr->sin_addr);
#else							/* CONFIG_NET_LWIP */
	target = (in_addr_t)addr->sin_addr.s_addr;
#endif							/* CONFIG_NET_LWIP */

	addr = (FAR struct sockaddr_in *)rtentry->rt_netmask;
#if CONFIG_NET_LWIP
	netmask = (in_addr_t)ip_2_ip4(addr->sin_addr);
#else							/* CONFIG_NET_LWIP */
	netmask = (in_addr_t)addr->sin_addr.s_addr;
#endif							/* CONFIG_NET_LWIP */

	/* The router is an optional argument */

	if (rtentry->rt_router) {
		addr = (FAR struct sockaddr_in *)rtentry->rt_router;
#if CONFIG_NET_LWIP
	    router = (in_addr_t)ip_2_ip4(addr->sin_addr);
#else							/* CONFIG_NET_LWIP */
	    router = (in_addr_t)addr->sin_addr.s_addr;
#endif							/* CONFIG_NET_LWIP */
	} else {
		router = 0;
	}

	return net_addroute(target, netmask, router);
}
#endif							/* CONFIG_NET_ROUTE && CONFIG_NET_IPv4 */

/****************************************************************************
 * Name: ioctl_addipv6route
 *
 * Description:
 *   Add an IPv6 route to the routing table.
 *
 * Input Parameters:
 *   rentry - Describes the route to be added
 *
 ****************************************************************************/

#if defined(CONFIG_NET_ROUTE) && defined(CONFIG_NET_IPv6)
static int ioctl_addipv6route(FAR struct rtentry *rtentry)
{
	FAR struct sockaddr_in6 *addr;
	net_ipv6addr_t target;
	net_ipv6addr_t netmask;
	net_ipv6addr_t router;

	addr = (FAR struct sockaddr_in6 *)rtentry->rt_target;
#if CONFIG_NET_LWIP
	target = (net_ipv6addr_t)ip_2_ip6(addr->sin6_addr);
#else							/* CONFIG_NET_LWIP */
	target = (net_ipv6addr_t)addr->sin6_addr.u6_addr16;
#endif							/* CONFIG_NET_LWIP */

	addr = (FAR struct sockaddr_in6 *)rtentry->rt_netmask;
#if CONFIG_NET_LWIP
	netmask = (net_ipv6addr_t)ip_2_ip6(addr->sin6_addr);
#else							/* CONFIG_NET_LWIP */
	netmask = (net_ipv6addr_t)addr->sin6_addr.u6_addr16;
#endif							/* CONFIG_NET_LWIP */

	/* The router is an optional argument */

	if (rtentry->rt_router) {
		addr = (FAR struct sockaddr_in6 *)rtentry->rt_router;
#if CONFIG_NET_LWIP
	    router = (net_ipv6addr_t)ip_2_ip6(addr->sin6_addr);
#else							/* CONFIG_NET_LWIP */
	    router = (net_ipv6addr_t)addr->sin6_addr.u6_addr16;
#endif							/* CONFIG_NET_LWIP */
	} else {
		router = NULL;
	}

	return net_addroute(target, netmask, router);
}
#endif							/* CONFIG_NET_ROUTE && CONFIG_NET_IPv6 */

/****************************************************************************
 * Name: ioctl_delipv4route
 *
 * Description:
 *   Delete an IPv4 route to the routing table.
 *
 * Input Parameters:
 *   rentry - Describes the route to be deleted
 *
 ****************************************************************************/

#if defined(CONFIG_NET_ROUTE) && defined(CONFIG_NET_IPv4)
static int ioctl_delipv4route(FAR struct rtentry *rtentry)
{
	FAR struct sockaddr_in *addr;
	in_addr_t target;
	in_addr_t netmask;

	addr = (FAR struct sockaddr_in *)rtentry->rt_target;
#if CONFIG_NET_LWIP
	target = (in_addr_t)ip_2_ip4(addr->sin_addr);
#else							/* CONFIG_NET_LWIP */
	target = (in_addr_t)addr->sin_addr.s_addr;
#endif							/* CONFIG_NET_LWIP */

	addr = (FAR struct sockaddr_in *)rtentry->rt_netmask;
#if CONFIG_NET_LWIP
	netmask = (in_addr_t)ip_2_ip4(addr->sin_addr);
#else							/* CONFIG_NET_LWIP */
	netmask = (in_addr_t)addr->sin_addr.s_addr;
#endif							/* CONFIG_NET_LWIP */

	return net_delroute(target, netmask);
}
#endif							/* CONFIG_NET_ROUTE && CONFIG_NET_IPv4 */

/****************************************************************************
 * Name: ioctl_delipv6route
 *
 * Description:
 *   Delete an IPv6 route to the routing table.
 *
 * Input Parameters:
 *   rentry - Describes the route to be deleted
 *
 ****************************************************************************/

#if defined(CONFIG_NET_ROUTE) && defined(CONFIG_NET_IPv6)
static int ioctl_delipv6route(FAR struct rtentry *rtentry)
{
	FAR struct sockaddr_in6 *addr;
	net_ipv6addr_t target;
	net_ipv6addr_t netmask;

	addr = (FAR struct sockaddr_in6 *)rtentry->rt_target;
#if CONFIG_NET_LWIP
	target = (net_ipv6addr_t)ip_2_ip6(addr->sin6_addr);
#else							/* CONFIG_NET_LWIP */
	target = (net_ipv6addr_t)addr->sin6_addr.u6_addr16;
#endif							/* CONFIG_NET_LWIP */

	addr = (FAR struct sockaddr_in6 *)rtentry->rt_netmask;
#if CONFIG_NET_LWIP
	netmask = (net_ipv6addr_t)ip_2_ip6(addr->sin6_addr);
#else							/* CONFIG_NET_LWIP */
	netmask = (net_ipv6addr_t)addr->sin6_addr.u6_addr16;
#endif							/* CONFIG_NET_LWIP */

	return net_delroute(target, netmask);
}
#endif							/* CONFIG_NET_ROUTE && CONFIG_NET_IPv6 */

/****************************************************************************
 * Name: ioctl_getipv4addr
 *
 * Description:
 *   Copy IP addresses from device structure to user memory.
 *
 * Input Parameters:
 *   outaddr - Pointer to the user-provided memory to receive the address.
 *   inaddr - The source IP address in the device structure.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_IPv4
static void ioctl_getipv4addr(FAR struct sockaddr *outaddr, in_addr_t inaddr)
{
	FAR struct sockaddr_in *dest = (FAR struct sockaddr_in *)outaddr;
	dest->sin_family = AF_INET;
	dest->sin_port = 0;
	dest->sin_addr.s_addr = inaddr;
}
#endif							/* CONFIG_NET_IPV4 */

/****************************************************************************
 * Name: ioctl_getipv6addr
 *
 * Description:
 *   Copy IP addresses from device structure to user memory.
 *
 * Input Parameters:
 *   outaddr - Pointer to the user-provided memory to receive the address.
 *   inaddr - The source IP adress in the device structure.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_IPv6
#ifdef CONFIG_NET_LWIP
static void ioctl_getipv6addr(FAR struct sockaddr_storage *outaddr, ip6_addr_t * inaddr)
#else							/* CONFIG_NET_LWIP */
static void ioctl_getipv6addr(FAR struct sockaddr_storage *outaddr, FAR const net_ipv6addr_t inaddr)
#endif							/* CONFIG_NET_LWIP */
{
	FAR struct sockaddr_in6 *dest = (FAR struct sockaddr_in6 *)outaddr;
	dest->sin6_family = AF_INET6;
	dest->sin6_port = 0;
	memcpy(dest->sin6_addr.s6_addr, inaddr, 16);
}
#endif							/* CONFIG_NET_IPV6 */

/****************************************************************************
 * Name: ioctl_setipv4addr
 *
 * Description:
 *   Copy IP addresses from user memory into the device structure
 *
 * Input Parameters:
 *   outaddr - Pointer to the source IP address in the device structure.
 *   inaddr - Pointer to the user-provided memory to containing the new IP
 *     address.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_IPv4
static void ioctl_setipv4addr(FAR in_addr_t *outaddr, FAR const struct sockaddr *inaddr)
{
	FAR const struct sockaddr_in *src = (FAR const struct sockaddr_in *)inaddr;
	*outaddr = src->sin_addr.s_addr;
}
#endif							/* CONFIG_NET_IPV4 */


/****************************************************************************
 * Name: ioctl_setipv6addr
 *
 * Description:
 *   Copy IP addresses from user memory into the device structure
 *
 * Input Parameters:
 *   outaddr - Pointer to the source IP address in the device structure.
 *   inaddr - Pointer to the user-provided memory to containing the new IP
 *     address.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_IPv6
#ifdef CONFIG_NET_LWIP
static void ioctl_setipv6addr(ip6_addr_t * outaddr, FAR const struct sockaddr_storage *inaddr)
#else							/* CONFIG_NET_LWIP */
static void ioctl_setipv6addr(FAR net_ipv6addr_t outaddr, FAR const struct sockaddr_storage *inaddr)
#endif							/* CONFIG_NET_LWIP */
{
	FAR const struct sockaddr_in6 *src = (FAR const struct sockaddr_in6 *)inaddr;
	memcpy(outaddr, src->sin6_addr.s6_addr, 16);
}
#endif							/* CONFIG_NET_IPV6 */

struct ifenum {
	FAR struct ifconf	*ifc;
	unsigned int		pos;
};

static int netdev_getconf(FAR struct netif *dev, void *arg)
{
	FAR struct ifenum *ifenum = (FAR struct ifenum *)arg;
	FAR struct ifconf *ifc = ifenum->ifc;
	FAR struct ifreq  *ifr = (FAR struct ifreq *)(ifc->ifc_buf + ifenum->pos);

	if (ifenum->pos + sizeof(struct ifreq) > ifc->ifc_len) {
		return -EFAULT;
	}

	strncpy(ifr->ifr_name, dev->d_ifname, IFNAMSIZ - 1);
	struct sockaddr_in *sin = (struct sockaddr_in *)&ifr->ifr_addr;
	sin->sin_addr.s_addr = ip4_addr_get_u32(ip_2_ip4(&dev->ip_addr));
	ifenum->pos += sizeof(struct ifreq);

	return OK;
}

static int ioctl_siocgifconf(FAR struct ifconf *ifc)
{
	int ret;
	struct ifenum ife = {
		.ifc = ifc,
		.pos = 0,
	};

	ret = netdev_foreach(netdev_getconf, (void *)&ife);
	if (ret == OK) {
		ifc->ifc_len = ife.pos;
	}

	return ret;
}

/****************************************************************************
 * Name: netdev_setipv6addr
 *
 * Description:
 *   Set IPv6 address to netdev and join multicast group
 *
 * Parameters:
 *   dev - NIC
 *   inaddr - the address to set
 *
 * Return:
 *
 ****************************************************************************/
#ifdef CONFIG_NET_IPv6
static void netdev_setipv6addr(struct netif *dev, FAR const struct sockaddr_storage *inaddr)
{
	ip6_addr_t temp;
	s8_t idx;

	if (!dev || !inaddr) {
		ndbg("Invalid parameters\n");
		return;
	}

	struct sockaddr_in6 *src = (struct sockaddr_in6 *)inaddr;
	memcpy(&temp, src->sin6_addr.s6_addr, 16);

	idx = netif_get_ip6_addr_match(dev, &temp);
	if (idx != -1) {
#ifdef CONFIG_NET_IPv6_MLD
		ip6_addr_t solicit_addr;

		/* leaving MLD6 group */
		ip6_addr_set_solicitednode(&solicit_addr, ip_2_ip6(&dev->ip6_addr[0])->addr[idx]);
		mld6_leavegroup_netif(dev, &solicit_addr);
		ndbg("MLD6 group left - %X : %X : %X : %X\n",
			 PP_HTONL(solicit_addr.addr[0]), PP_HTONL(solicit_addr.addr[1]),
			 PP_HTONL(solicit_addr.addr[2]), PP_HTONL(solicit_addr.addr[3]));
#endif /* CONFIG_NET_IPv6_MLD */
		/* delete static ipv6 address if the same ip address exists */
		netif_ip6_addr_set_state(dev, idx, IP6_ADDR_INVALID);
		return;
	}

#ifdef CONFIG_NET_IPv6_AUTOCONFIG
	/* enable IPv6 address stateless auto-configuration */
	netif_set_ip6_autoconfig_enabled(dev, 1);
#endif /* CONFIG_NET_IPv6_AUTOCONFIG */

	/* add static ipv6 address */
	(void)netif_add_ip6_address(dev, &temp, &idx);

#ifdef CONFIG_NET_IPv6_MLD
	ip6_addr_t solicit_addr;

	/* set MLD6 group to receive solicit multicast message */
	ip6_addr_set_solicitednode(&solicit_addr, ip_2_ip6(&dev->ip6_addr[0])->addr[idx]);
	mld6_joingroup_netif(dev, &solicit_addr);
	ndbg("MLD6 group added - %X : %X : %X : %X\n",
		 PP_HTONL(solicit_addr.addr[0]), PP_HTONL(solicit_addr.addr[1]),
		 PP_HTONL(solicit_addr.addr[2]), PP_HTONL(solicit_addr.addr[3]));
#endif /* CONFIG_NET_IPv6_MLD */

#ifdef CONFIG_NET_LWIP
	ioctl_setipv6addr(ip_2_ip6(&dev->ip_addr), inaddr);
#else							/* CONFIG_NET_LWIP */
	ioctl_setipv6addr(dev->d_ipv6addr, inaddr);
#endif							/* CONFIG_NET_LWIP */
	return;
}
#endif // CONFIG_NET_IPv6
/****************************************************************************
 * Name: netdev_ifrdev
 *
 * Description:
 *   Verify the struct ifreq and get the Ethernet device.
 *
 * Parameters:
 *   req - The argument of the ioctl cmd
 *
 * Return:
 *  A pointer to the driver structure on success; NULL on failure.
 *
 ****************************************************************************/

static FAR struct netif *netdev_ifrdev(FAR struct ifreq *req)
{
	if (!req) {
		return NULL;
	}

	/* Find the network device associated with the device name
	 * in the request data.
	 */

	return netdev_findbyname(req->ifr_name);
}

/****************************************************************************
 * Name: netdev_ifrioctl
 *
 * Description:
 *   Perform network device specific operations.
 *
 * Parameters:
 *   psock    Socket structure
 *   dev      Ethernet driver device structure
 *   cmd      The ioctl command
 *   req      The argument of the ioctl cmd
 *
 * Return:
 *   >=0 on success (positive non-zero values are cmd-specific)
 *   Negated errno returned on failure.
 *
 ****************************************************************************/

static int netdev_ifrioctl(int cmd, FAR struct ifreq *req)
{
	FAR struct netif *dev;
	int ret = -EINVAL;

	//nvdbg("cmd: %d\n", cmd);

	/* Execute the command */

	switch (cmd) {
#ifdef CONFIG_NET_IPv4
	case SIOCGIFADDR: {			/* Get IP address */
		dev = netdev_ifrdev(req);
		if (dev) {
			ioctl_getipv4addr(&req->ifr_addr, ip4_addr_get_u32(ip_2_ip4(&dev->ip_addr)));
			ret = OK;
		}
	}
	break;
	case SIOCSIFADDR: {			/* Set IP address */
		dev = netdev_ifrdev(req);
		if (dev) {
			netdev_soft_ifdown(dev);
#ifdef CONFIG_NET_LWIP
			ip_addr_t ipaddr, netmask, gw;
			ioctl_setipv4addr(&ip4_addr_get_u32(ip_2_ip4(&ipaddr)), &req->ifr_addr);
			netmask = dev->netmask;
			gw = dev->gw;
			netifapi_netif_set_addr(dev, ip_2_ip4(&ipaddr), ip_2_ip4(&netmask), ip_2_ip4(&gw));
#endif							/* CONFIG_NET_LWIP */
			netdev_soft_ifup(dev);
			ret = OK;
		}
	}
	break;

	case SIOCGIFDSTADDR: {		/* Get P-to-P address */
		dev = netdev_ifrdev(req);
		if (dev) {
			ioctl_getipv4addr(&req->ifr_dstaddr, ip4_addr_get_u32(ip_2_ip4(&dev->gw)));
			ret = OK;
		}
	}
	break;

	case SIOCSIFDSTADDR: {		/* Set P-to-P address */
		dev = netdev_ifrdev(req);
		if (dev) {
#ifdef CONFIG_NET_LWIP
			netifapi_netif_set_down(dev);
			ip_addr_t ipaddr, netmask, gw;
			ioctl_setipv4addr(&ip4_addr_get_u32(ip_2_ip4(&gw)), &req->ifr_dstaddr);
			ipaddr = dev->ip_addr;
			netmask = dev->netmask;
			netifapi_netif_set_addr(dev, ip_2_ip4(&ipaddr), ip_2_ip4(&netmask), ip_2_ip4(&gw));
			netifapi_netif_set_up(dev);

			ret = OK;
#endif							/* CONFIG_NET_LWIP */
		}
	}
	break;

	case SIOCGIFBRDADDR:		/* Get broadcast IP address */
	case SIOCSIFBRDADDR: {		/* Set broadcast IP address */
		ret = -ENOSYS;
	}
	break;

	case SIOCGIFNETMASK: {		/* Get network mask */
		dev = netdev_ifrdev(req);
		if (dev) {
			ioctl_getipv4addr(&req->ifr_addr, ip4_addr_get_u32(ip_2_ip4(&dev->netmask)));
			ret = OK;
		}
	}
	break;

	case SIOCSIFNETMASK: {		/* Set network mask */
		dev = netdev_ifrdev(req);
		if (dev) {
#ifdef CONFIG_NET_LWIP
			netifapi_netif_set_down(dev);
			ip_addr_t ipaddr, netmask, gw;
			ioctl_setipv4addr(&ip4_addr_get_u32(ip_2_ip4(&netmask)), &req->ifr_addr);
			ipaddr = dev->ip_addr;
			gw = dev->gw;
			netifapi_netif_set_addr(dev, ip_2_ip4(&ipaddr), ip_2_ip4(&netmask), ip_2_ip4(&gw));
			netifapi_netif_set_up(dev);

			ret = OK;
#endif							/* CONFIG_NET_LWIP */
		}
	}
	break;
#endif							/* CONFIG_NET_IPV4 */

	/* TODO: Support IPv6 related IOCTL calls once IPv6 is functional */
#ifdef CONFIG_NET_IPv6
	case SIOCGLIFADDR: {		/* Get IP address */
		dev = netdev_ifrdev(req);
		if (dev) {
			FAR struct lifreq *lreq = (FAR struct lifreq *)req;
#ifdef CONFIG_NET_LWIP
			ioctl_getipv6addr(&lreq->lifr_addr, ip_2_ip6(&dev->ip_addr));
#else							/* CONFIG_NET_LWIP */
			ioctl_getipv6addr(&lreq->lifr_addr, dev->d_ipv6addr);
#endif							/* CONFIG_NET_LWIP */
			ret = OK;
		}
	}
	break;
	case SIOCSLIFADDR: {		/* Set IP address */
		dev = netdev_ifrdev(req);
		if (dev) {
			FAR struct lifreq *lreq = (FAR struct lifreq *)req;
			netdev_soft_ifdown(dev);
			netdev_setipv6addr(dev, &lreq->lifr_addr);
			netdev_soft_ifup(dev);
			ret = OK;
		}
	}
	break;
	case SIOCGLIFDSTADDR: {		/* Get P-to-P address */
		dev = netdev_ifrdev(req);
		if (dev) {
			FAR struct lifreq *lreq = (FAR struct lifreq *)req;
#ifdef CONFIG_NET_LWIP
			ioctl_getipv6addr(&lreq->lifr_dstaddr, ip_2_ip6(&dev->gw));
#else							/* CONFIG_NET_LWIP */
			ioctl_getipv6addr(&lreq->lifr_dstaddr, dev->d_ipv6draddr);
#endif							/* CONFIG_NET_LWIP */
			ret = OK;
		}
	}
	break;

	case SIOCSLIFDSTADDR: {		/* Set P-to-P address */
		dev = netdev_ifrdev(req);
		if (dev) {
			FAR struct lifreq *lreq = (FAR struct lifreq *)req;
#ifdef CONFIG_NET_LWIP
			ioctl_setipv6addr(ip_2_ip6(&dev->gw), &lreq->lifr_dstaddr);
#else							/* CONFIG_NET_LWIP */
			ioctl_setipv6addr(dev->d_ipv6draddr, &lreq->lifr_dstaddr);
#endif							/* CONFIG_NET_LWIP */
			ret = OK;
		}
	}
	break;

	case SIOCGLIFBRDADDR:		/* Get broadcast IP address */

	case SIOCSLIFBRDADDR: {		/* Set broadcast IP address */
		ret = -ENOSYS;
	}
	break;

	case SIOCGLIFNETMASK: {		/* Get network mask */
		dev = netdev_ifrdev(req);
		if (dev) {
			FAR struct lifreq *lreq = (FAR struct lifreq *)req;
#ifdef CONFIG_NET_LWIP
			ioctl_getipv6addr(&lreq->lifr_addr, ip_2_ip6(&dev->netmask));
#else							/* CONFIG_NET_LWIP */
			ioctl_getipv6addr(&lreq->lifr_addr, dev->d_ipv6netmask);
#endif							/* CONFIG_NET_LWIP */
			ret = OK;
		}
	}
	break;

	case SIOCSLIFNETMASK: {		/* Set network mask */
		dev = netdev_ifrdev(req);
		if (dev) {
			FAR struct lifreq *lreq = (FAR struct lifreq *)req;
#ifdef CONFIG_NET_LWIP
			ioctl_setipv6addr(ip_2_ip6(&dev->netmask), &lreq->lifr_addr);
#else							/* CONFIG_NET_LWIP */
			ioctl_setipv6addr(dev->d_ipv6netmask, &lreq->lifr_addr);
#endif							/* CONFIG_NET_LWIP */
			ret = OK;
		}
	}
	break;
#endif							/* CONFIG_NET_IPv6 */

	case SIOCGLIFMTU:			/* Get MTU size */
	case SIOCGIFMTU: {			/* Get MTU size */
		dev = netdev_ifrdev(req);
		if (dev) {
			req->ifr_mtu = NET_DEV_MTU(dev);
			ret = OK;
		}
	}
	break;

	case SIOCSIFFLAGS: {		/* Sets the interface flags */
		/* Is this a request to bring the interface up? */

		dev = netdev_ifrdev(req);
		if (dev) {
			if (req->ifr_flags & IFF_UP) {
				/* Yes.. bring the interface up */

				netdev_ifup(dev);
			}

			/* Is this a request to take the interface down? */

			else if (req->ifr_flags & IFF_DOWN) {
				/* Yes.. take the interface down */
				netdev_ifdown(dev);
			}
		}

		ret = OK;
	}
	break;

	case SIOCGIFFLAGS: {		/* Gets the interface flags */
		dev = netdev_ifrdev(req);
		if (dev) {
			req->ifr_flags = dev->d_flags;
		}

		ret = OK;
	}
	break;

	/* MAC address operations only make sense if Ethernet is supported */
#ifdef CONFIG_NET_ETHERNET
	case SIOCGIFHWADDR: {		/* Get hardware address */
		dev = netdev_ifrdev(req);
		if (dev) {
			req->ifr_hwaddr.sa_family = AF_INETX;
			memcpy(req->ifr_hwaddr.sa_data, dev->d_mac.ether_addr_octet, IFHWADDRLEN);
			ret = OK;
		}
	}
	break;

	case SIOCSIFHWADDR: {		/* Set hardware address -- will not take effect until ifup */
		dev = netdev_ifrdev(req);
		if (dev) {
			memcpy(dev->d_mac.ether_addr_octet, req->ifr_hwaddr.sa_data, IFHWADDRLEN);
			ret = OK;
		}
	}
	break;
#endif							/* CONFIG_NET_EHTERNET */

	case SIOCDIFADDR: {			/* Delete IP address */
		dev = netdev_ifrdev(req);
		if (dev) {
#ifdef CONFIG_NET_LWIP
			netif_set_down(dev);
#endif
#ifdef CONFIG_NET_IPv4
			dev->d_ipaddr = 0;
#endif							/* CONFIG_NET_IPV4 */
#ifdef CONFIG_NET_IPv6
#ifdef CONFIG_NET_LWIP
			memset(ip_2_ip6(&dev->ip_addr), 0, sizeof(net_ipv6addr_t));
#else
			memset(&dev->d_ipv6addr, 0, sizeof(net_ipv6addr_t));
#endif							/* CONFIG_NET_LWIP */
#endif							/* CONFIG_NET_IPV6 */
			ret = OK;
		}
	}
	break;

	case SIOCGIFCOUNT: {		/* Get number of devices */
		req->ifr_count = netdev_count();
		ret = -ENOSYS;
	}
	break;

#ifdef CONFIG_NET_ARPIOCTLS
	case SIOCSARP:				/* Set a ARP mapping */
	case SIOCDARP:				/* Delete an ARP mapping */
	case SIOCGARP:				/* Get an ARP mapping */
#error "IOCTL Commands not implemented"
#endif							/* CONFIG_NET_ARPIOCTLS */

#ifdef CONFIG_NETDEV_PHY_IOCTL
#ifdef CONFIG_ARCH_PHY_INTERRUPT
	case SIOCMIINOTIFY: {		/* Set up for PHY event notifications */
		dev = netdev_ifrdev(req);
		if (dev && dev->d_ioctl) {
			struct mii_iotcl_notify_s *notify = &req->ifr_ifru.ifru_mii_notify;
			ret = dev->d_ioctl(dev, cmd, ((long)(uintptr_t)notify));
		}
	}
	break;
#endif							/* CONFIG_ARCH_PHY_INTERRUPT */

	case SIOCGMIIPHY:			/* Get address of MII PHY in use */
	case SIOCGMIIREG:			/* Get MII register via MDIO */
	case SIOCSMIIREG: {			/* Set MII register via MDIO */
		dev = netdev_ifrdev(req);
		if (dev && dev->d_ioctl) {
			struct mii_ioctl_data_s *mii_data = &req->ifr_ifru.ifru_mii_data;
			ret = dev->d_ioctl(dev, cmd, ((long)(uintptr_t)mii_data));
		}
	}
	break;
#endif							/* CONFIG_NETDEV_PHY_IOCTL */
	case SIOCGIFCONF:
		ret = ioctl_siocgifconf((FAR struct ifconf *)req);
		break;
	default: {
		ret = -ENOTTY;
	}
	break;;
	}

	return ret;
}

/****************************************************************************
 * Name: netdev_imsfdev
 *
 * Description:
 *   Verify the struct ip_msfilter and get the Ethernet device.
 *
 * Parameters:
 *   req - The argument of the ioctl cmd
 *
 * Return:
 *  A pointer to the driver structure on success; NULL on failure.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_IGMP
static FAR struct netif *netdev_imsfdev(FAR struct ip_msfilter *imsf)
{
	if (!imsf) {
		return NULL;
	}

	/* Find the network device associated with the device name
	 * in the request data.
	 */

	return netdev_findbyname(imsf->imsf_name);
}
#endif							/* CONFIG_NET_IGMP */

/****************************************************************************
 * Name: netdev_nmioctl
 *
 * Description:
 *   Perform network monitor specific operations.
 *
 * Parameters:
 *   sock    Socket structure
 *   cmd     The ioctl command
 *   arg    The argument of ioctl command
 *
 * Return:
 *   >=0 on success (positive non-zero values are cmd-specific)
 *   Negated errno returned on failure.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_NETMON
static int netdev_nmioctl(int cmd, void  *arg)
{
	int ret = -EINVAL;
	switch (cmd) {
	case SIOCGETSOCK:          /* Get socket info. */
		// monitoring socket is not supported without network manager
		// please turn on CONFIG_NETMON
		ret = -EINVAL;
		break;
#ifdef CONFIG_NET_STATS
	case SIOCGDSTATS:          /* Get netdev info. */
		ret = netdev_getstats(arg);
		break;
#endif
	default:
		ret = -ENOTTY;
		break;
	} /* end switch */

	return ret;
}
#endif                            /* CONFIG_NET_NETMON */


/****************************************************************************
 * Name: netdev_imsfioctl
 *
 * Description:
 *   Perform network device specific operations.
 *
 * Parameters:
 *   psock    Socket structure
 *   dev      Ethernet driver device structure
 *   cmd      The ioctl command
 *   imsf     The argument of the ioctl cmd
 *
 * Return:
 *   >=0 on success (positive non-zero values are cmd-specific)
 *   Negated errno returned on failure.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_IGMP
static int netdev_imsfioctl(int cmd, FAR struct ip_msfilter *imsf)
{
	FAR struct netif *dev;
	int ret = -EINVAL;

	nvdbg("cmd: %d\n", cmd);

	/* Execute the command */

	switch (cmd) {
	case SIOCSIPMSFILTER: {		/* Set source filter content */
		dev = netdev_imsfdev(imsf);
		if (dev) {
			if (imsf->imsf_fmode == MCAST_INCLUDE) {
				ret = igmp_joingroup(dev->ip_addr, &imsf->imsf_multiaddr);
			} else {
				DEBUGASSERT(imsf->imsf_fmode == MCAST_EXCLUDE);
				ret = igmp_leavegroup(dev->ip_addr, &imsf->imsf_multiaddr);
			}
		}
	}
	break;

	case SIOCGIPMSFILTER:		/* Retrieve source filter addresses */
	default:
		ret = -ENOTTY;
		break;
	}

	return ret;
}
#endif							/* CONFIG_NET_IGMP */

/****************************************************************************
 * Name: netdev_rtioctl
 *
 * Description:
 *   Perform routing table specific operations.
 *
 * Parameters:
 *   psock    Socket structure
 *   dev      Ethernet driver device structure
 *   cmd      The ioctl command
 *   rtentry  The argument of the ioctl cmd
 *
 * Return:
 *   >=0 on success (positive non-zero values are cmd-specific)
 *   Negated errno returned on failure.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_ROUTE
static int netdev_rtioctl(int cmd, FAR struct rtentry *rtentry)
{
	int ret = -EAFNOSUPPORT;

	/* Execute the command */

	switch (cmd) {
	case SIOCADDRT: {			/* Add an entry to the routing table */
		/* The target address and the netmask are required values */

		if (!rtentry || !rtentry->rt_target || !rtentry->rt_netmask) {
			return -EINVAL;
		}
#ifdef CONFIG_NET_IPv4
#ifdef CONFIG_NET_IPv6
		if (rtentry->rt_target->ss_family == AF_INET)
#endif							/* CONFIG_NET_IPV6 */
		{
			ret = ioctl_addipv4route(rtentry);
		}
#endif							/* CONFIG_NET_IPv4 */

#ifdef CONFIG_NET_IPv6
#ifdef CONFIG_NET_IPv4
		else
#endif							/* CONFIG_NET_IPV4 */
		{
			ret = ioctl_addipv6route(rtentry);
		}
#endif							/* CONFIG_NET_IPv6 */
	}
	break;

	case SIOCDELRT: {			/* Delete an entry from the routing table */
		/* The target address and the netmask are required values */

		if (!rtentry || !rtentry->rt_target || !rtentry->rt_netmask) {
			return -EINVAL;
		}
#ifdef CONFIG_NET_IPv4
#ifdef CONFIG_NET_IPv6
		if (rtentry->rt_target->ss_family == AF_INET)
#endif							/* CONFIG_NET_IPV6 */
		{
			ret = ioctl_delipv4route(rtentry);
		}
#endif							/* CONFIG_NET_IPv4 */

#ifdef CONFIG_NET_IPv6
#ifdef CONFIG_NET_IPv4
		else
#endif							/* CONFIG_NET_IPV4 */
		{
			ret = ioctl_delipv6route(rtentry);
		}
#endif							/* CONFIG_NET_IPv6 */
	}
	break;

	default:
		ret = -ENOTTY;
		break;
	}

	return ret;
}
#endif							/* CONFIG_NET_ROUTE */

#if CONFIG_NET_LWIP
static struct addrinfo *copy_addrinfo(struct addrinfo *src)
{
	struct addrinfo *tmp = src;
	struct addrinfo *prev = NULL;
	struct addrinfo *root = NULL;
	int canonname_len = 0;
	while (tmp) {
		struct addrinfo *dst = NULL;
		dst = (struct addrinfo *)kumm_malloc(sizeof(struct addrinfo));
		if (!dst) {
			ndbg("copy_addrinfo() kumm_malloc failed\n");
			break;
		}
		dst->ai_flags = tmp->ai_flags;
		dst->ai_family = tmp->ai_family;
		dst->ai_socktype = tmp->ai_socktype;
		dst->ai_protocol = tmp->ai_protocol;
		dst->ai_addrlen = tmp->ai_addrlen;

		dst->ai_addr = (struct sockaddr *)kumm_malloc(sizeof(struct sockaddr));
		if (!dst->ai_addr) {
			ndbg("copy_addrinfo() kumm_malloc failed\n");
			kumm_free(dst);
			break;
		}

		memcpy(dst->ai_addr, tmp->ai_addr, sizeof(struct sockaddr));

		if (tmp->ai_canonname) {
			canonname_len = strlen(tmp->ai_canonname) + 1;
			dst->ai_canonname = (char *)kumm_malloc(canonname_len);
			if (!dst->ai_canonname) {
				ndbg("copy_addrinfo() kumm_malloc failed\n");
				kumm_free(dst->ai_addr);
				kumm_free(dst);
				break;
			}
			memcpy(dst->ai_canonname, tmp->ai_canonname, canonname_len);
		} else {
			dst->ai_canonname = NULL;
		}

		dst->ai_next = NULL;
		if (prev) {
			prev->ai_next = dst;
		} else {
			root = dst;
		}
		tmp = tmp->ai_next;
		prev = dst;
	}

	return root;
}

static int free_addrinfo(struct addrinfo *ai)
{
	struct addrinfo *next;

	while (ai != NULL) {
		next = ai->ai_next;
		kumm_free(ai);
		ai = next;
	}
	return 0;
}

#if defined(CONFIG_NET_LWIP_DHCP)
#define DHCPD_MQ_NAME "dhcpd_queue"
#define DHCPD_MQ_LEN 11
#define DHCPD_MQ_MAX_LEN 20

static void _dhcpd_join(dhcp_evt_type_e type, void *data)
{
	ndbg("dhcpd joined");

	struct mq_attr attr;
	attr.mq_maxmsg = DHCPD_MQ_MAX_LEN;
	attr.mq_msgsize = DHCPD_MQ_LEN;
	attr.mq_flags = 0;
	attr.mq_curmsgs = 0;

	mqd_t md = mq_open(DHCPD_MQ_NAME, O_RDWR | O_CREAT, 0666, &attr);
	if (!md) {
		ndbg("mq open fail (errno %d)\n", errno);
		mq_close(md);
		return;
	}

	char msg[DHCPD_MQ_LEN];
	dhcp_node_s *node = (dhcp_node_s *)data;
	msg[0] = 1;
	memcpy(&msg[1], &node->ipaddr, 4);
	memcpy(&msg[5], &node->macaddr, 6);
	int mq_ret = mq_send(md, msg, DHCPD_MQ_LEN, 100);
	if (mq_ret < 0) {
		ndbg("send mq fail (errno %d)\n", errno);
	}

	mq_close(md);

	return;
}

#endif

/****************************************************************************
 * Function: lwip_func_ioctl
 *
 * Description:
 *   Call lwip API
 *
 * Parameters:
 *   s        Descriptor of device
 *   cmd      The ioctl command
 *   arg      Type of the information to get
 *
 * Returned Value:
 *   0 on success, negated errno on failure.
 *
 ****************************************************************************/
int lwip_func_ioctl(int s, int cmd, void *arg)
{
	int ret = -EINVAL;
	struct lwip_sock *sock = get_socket(s);
	if (!sock) {
		ret = -EBADF;
		return ret;
	}
	struct req_lwip_data *in_arg = (struct req_lwip_data *)arg;
	if (!in_arg) {
		return ret;
	}

	struct addrinfo *res = NULL;
	struct hostent *host_ent = NULL;
	struct hostent *user_ent = NULL;

	switch (in_arg->type) {
#if LWIP_DNS
	case GETADDRINFO:
		in_arg->req_res = lwip_getaddrinfo(in_arg->host_name, in_arg->serv_name, in_arg->ai_hint, &res);
		if (in_arg->req_res != 0) {
			ndbg("lwip_getaddrinfo() returned with the error code: %d\n", ret);
			in_arg->ai_res = NULL;
			ret = -EINVAL;
		} else {
			in_arg->ai_res = copy_addrinfo(res);
			ret = OK;
		}
		break;
	case FREEADDRINFO:
		in_arg->req_res = free_addrinfo(in_arg->ai);
		ret = OK;
		break;
	case DNSSETSERVER:
		dns_setserver(in_arg->num_dns, in_arg->dns_server);
		ret = OK;
		break;
	case GETHOSTBYNAME:
		host_ent = lwip_gethostbyname(in_arg->host_name);
		if (!host_ent) {
			ndbg("lwip_gethostbyname() returned with the error code: %d\n", HOST_NOT_FOUND);
			ret = -EINVAL;
		} else {
			user_ent = in_arg->host_entry;
			strncpy(user_ent->h_name, host_ent->h_name, DNS_MAX_NAME_LENGTH);
			user_ent->h_name[DNS_MAX_NAME_LENGTH] = 0;
			memcpy(user_ent->h_addr_list[0], host_ent->h_addr_list[0], sizeof(struct in_addr));
			user_ent->h_addrtype = host_ent->h_addrtype;
			user_ent->h_length = host_ent->h_length;

			ret	= OK;
		}
		break;
	case GETNAMEINFO:
		in_arg->req_res = lwip_getnameinfo(in_arg->sa, in_arg->sa_len, (char *)in_arg->host_name, in_arg->host_len, (char *)in_arg->serv_name, in_arg->serv_len, in_arg->flags);
		if (in_arg->req_res != 0) {
			ndbg("lwip_getnameinfo() returned with the error code: %d\n", ret);
			ret = -EINVAL;
		} else {
			ret = OK;
		}
		break;
#endif
#if defined(CONFIG_NET_LWIP_DHCP)
#if defined(CONFIG_LWIP_DHCPC)
	case DHCPCSTART:
		in_arg->req_res = netdev_dhcp_client_start((const char *)in_arg->intf);
		if (in_arg->req_res != 0) {
			ret = -EINVAL;
			ndbg("start dhcp fail\n");
		} else {
			ret = OK;
		}
		break;
	case DHCPCSTOP:
		netdev_dhcp_client_stop((const char *)in_arg->intf);
		in_arg->req_res = 0;
		ret = OK;
		break;
#endif
#if defined(CONFIG_LWIP_DHCPS)
	case DHCPDSTART:
		in_arg->req_res = netdev_dhcp_server_start((char *)in_arg->intf, _dhcpd_join);
		if (in_arg->req_res != 0) {
			ret = -EINVAL;
			ndbg("start dhcpd fail\n");
		} else {
			ret = OK;
		}
		break;
	case DHCPDSTOP:
		in_arg->req_res = netdev_dhcp_server_stop((char *)in_arg->intf);
		if (in_arg->req_res != 0) {
			ret = -EINVAL;
			ndbg("stop dhcpd fail\n");
		} else {
			ret = OK;
		}
		break;
	case DHCPDSTATUS:
		in_arg->req_res = netdev_dhcp_server_status((char *)in_arg->intf);
		if (in_arg->req_res != 0) {
			ret = -EINVAL;
			ndbg("stop dhcpd fail\n");
		} else {
			ret = OK;
		}
		break;
#endif
#endif
	default:
		ndbg("Wrong request type: %d\n", in_arg->type);
		break;
	}

	return ret;
}

/****************************************************************************
 * Function: lwipioctl
 *
 * Description:
 *   Call lwip_ioctl() with FIONREAD/FIONBIO commands or
 *   call lwip API with SIOCLWIP command
 *
 * Parameters:
 *   sockfd   Socket file descriptor
 *   cmd      The ioctl command
 *   arg      Type of the information to get
 *
 * Returned Value:
 *   0 on success, negated errno on failure.
 *
 ****************************************************************************/

int lwipioctl(int sockfd, int cmd, void *arg)
{
	int ret = -ENOTTY;

	if (cmd == FIONREAD || cmd == FIONBIO) {
		ret = lwip_ioctl(sockfd, (long)cmd, arg);
		if (ret == -1) {
			return -get_errno();
		}
	} else if (cmd == SIOCLWIP) {
		return lwip_func_ioctl(sockfd, cmd, arg);
	}

	return ret;
}
#endif							/* CONFIG_NET_LWIP */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netdev_ioctl
 *
 * Description:
 *   Perform network device specific operations.
 *
 * Parameters:
 *   sockfd   Socket descriptor of device
 *   cmd      The ioctl command
 *   arg      The argument of the ioctl cmd
 *
 * Return:
 *   >=0 on success (positive non-zero values are cmd-specific)
 *   On a failure, -1 is returned with errno set appropriately
 *
 *   EBADF
 *     'sockfd' is not a valid descriptor.
 *   EFAULT
 *     'arg' references an inaccessible memory area.
 *   ENOTTY
 *     'cmd' not valid.
 *   EINVAL
 *     'arg' is not valid.
 *   ENOTTY
 *     'sockfd' is not associated with a network device.
 *   ENOTTY
 *      The specified request does not apply to the kind of object that the
 *      descriptor 'sockfd' references.
 *
 ****************************************************************************/

int net_ioctl(int sockfd, int cmd, unsigned long arg)
{
	/* FAR struct lwip_sock *sock = NULL; */
	int ret = -ENOTTY;

	/* Check if this is a valid command.  In all cases, arg is a pointer that has
	 * been cast to unsigned long.  Verify that the value of the to-be-pointer is
	 * non-NULL.
	 */

	if (!((cmd == FIONREAD) || (cmd == FIONBIO) || (_SIOCVALID(cmd)))) {
		ret = -ENOTTY;
		goto errout;
	}

	/* Verify that the sockfd corresponds to valid, allocated socket */
    /*  I am confused if sockfd were invalid then the request couldn't be reach here */
	/*  So checking validation of socket would be unneccessary */
	/* sock = get_socket(sockfd); */

	/*
	if (NULL == sock) {
		ret = -EBADF;
		goto errout;
	}
	*/

	/* Execute the command */
#ifdef CONFIG_NET_LWIP
	ret = lwipioctl(sockfd, cmd, (void *)((uintptr_t)arg));
#endif
	if (ret == -ENOTTY) {
		ret = netdev_ifrioctl(cmd, (FAR struct ifreq *)((uintptr_t)arg));
	}
#ifdef CONFIG_NET_NETMON
	if (ret == -ENOTTY) {
		ret = netdev_nmioctl(cmd, (void *)((uintptr_t)arg));
	}
#endif                          /* CONFIG_NET_NETMON */
#ifdef CONFIG_NET_IGMP
	if (ret == -ENOTTY) {
		ret = netdev_imsfioctl(cmd, (FAR struct ip_msfilter *)((uintptr_t)arg));
	}
#endif							/* CONFIG_NET_IGMP */
#ifdef CONFIG_NET_ROUTE
	if (ret == -ENOTTY) {
		ret = netdev_rtioctl(cmd, (FAR struct rtentry *)((uintptr_t)arg));
	}
#endif							/* CONFIG_NET_ROUTE */

	/* Check for success or failure */

	if (ret >= 0) {
		return ret;
	}

	/* On failure, set the errno and return -1 */

errout:
	errno = -ret;
	return ERROR;
}

/****************************************************************************
 * Name: netdev_ifup / netdev_ifdown
 *
 * Description:
 *   Bring the interface up/down
 *
 ****************************************************************************/

void netdev_ifup(FAR struct netif *dev)
{
	/* Make sure that the device supports the d_ifup() method */

	if (!dev->d_ifup) {
		return;
	}

	/* Is the interface already up? */
	if ((dev->d_flags & IFF_UP) != 0) {
		/* No, bring the interface up now */
		return;
	}

	if (dev->d_ifup(dev) != OK) {
		ndbg("fail to ifup %s:%d\n", __FILE__, __LINE__);
		return;
	}

	/* Mark the interface as up */
	dev->d_flags |= IFF_UP;

	/* netdev_ifdown is a blocked function, but some API that is mapping d_ifup
	 * operated as non-blocking, so disalbe netdev_ifdown temporarily until API is fixed
	 */
	sleep(3);
	err_t res = netifapi_netif_set_up(dev);
	if (res != ERR_OK) {
		ndbg("netdev soft if up fail\n");
	}

	res = netifapi_netif_set_link_up(dev);
	if (res != ERR_OK) {
		ndbg("netdev soft link up fail\n");
	}

	/* Below logic is not processed by lwIP thread.
	 * So it can cause conflict to lwIP thread later.
	 * But now there are no APIs that can manage IPv6 auto-config.
	 * this will be handled carefully later.
	 */
#ifdef CONFIG_NET_IPv6
	/* IPV6 auto configuration : Link-Local address */
	nvdbg("IPV6 link local address auto config\n");

#ifdef CONFIG_NET_IPv6_AUTOCONFIG
	/* enable IPv6 address stateless auto-configuration */
	netif_set_ip6_autoconfig_enabled(dev, 1);
#endif /* CONFIG_NET_IPv6_AUTOCONFIG */
	/* To auto-config linklocal address, dev should have mac address already */
	netif_create_ip6_linklocal_address(dev, 1);
	ndbg("generated IPV6 linklocal address - %X : %X : %X : %X\n",
		 PP_HTONL(ip_2_ip6(&dev->ip6_addr[0])->addr[0]), PP_HTONL(ip_2_ip6(&dev->ip6_addr[0])->addr[1]), PP_HTONL(ip_2_ip6(&dev->ip6_addr[0])->addr[2]), PP_HTONL(ip_2_ip6(&dev->ip6_addr[0])->addr[3]));
#ifdef CONFIG_NET_IPv6_MLD
	ip6_addr_t solicit_addr;

	/* set MLD6 group to receive solicit multicast message */
	ip6_addr_set_solicitednode(&solicit_addr, ip_2_ip6(&dev->ip6_addr[0])->addr[3]);
	mld6_joingroup_netif(dev, &solicit_addr);
	ndbg("MLD6 group added - %X : %X : %X : %X\n",
		 PP_HTONL(solicit_addr.addr[0]), PP_HTONL(solicit_addr.addr[1]),
		 PP_HTONL(solicit_addr.addr[2]), PP_HTONL(solicit_addr.addr[3]));
#endif /* CONFIG_NET_IPv6_MLD */

#endif /* CONFIG_NET_IPv6 */
}


void netdev_ifdown(FAR struct netif *dev)
{
	/* Make sure that the device supports the d_ifdown() method */
	if (!dev->d_ifdown) {
		return;
	}

	/* Is the interface already down? */
	err_t res = netifapi_netif_set_link_down(dev);
	if (res != ERR_OK) {
		ndbg("netdev soft link down fail\n");
	}

	res = netifapi_netif_set_down(dev);
	if (res != ERR_OK) {
		ndbg("netdev soft if down fail\n");
	}

	if ((dev->d_flags & IFF_UP) == 0) {
		/* No, take the interface down now */
		return;
	}

	if (dev->d_ifdown(dev) == OK) {
		/* Mark the interface as down */
		ndbg("fail to ifdown %s:%d\n", __FILE__, __LINE__);
		dev->d_flags &= ~IFF_UP;
	}
}
#endif	/* CONFIG_NET && CONFIG_NSOCKET_DESCRIPTORS */
