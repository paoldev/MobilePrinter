#pragma once

class CFirewallHelper
{
public:

	//Register a Firewall inbount private TCP rule, given its name, its associated executable and related local ports.
	//If i_pwszExecutablePath is nullptr, the executable path of the running application is used.
	//If i_pwszLocalPorts is nullptr, all local ports are bound to the rule.
	static bool AddInboundPrivateTCPRule(const wchar_t* i_pwszRuleName, const wchar_t* i_pwszExecutablePath, const wchar_t* i_pwszLocalPorts);

	//Remove a Firewall rule given its name.
	static bool RemoveRule(const wchar_t* i_pwszRuleName);
};
