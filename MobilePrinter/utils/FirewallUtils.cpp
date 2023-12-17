//// 
//// This firewall code is derived from the sample at
//// https://docs.microsoft.com/en-us/previous-versions/windows/desktop/ics/c-adding-an-outbound-rule
//// 

#include "pch.h"
#include "FirewallUtils.h"
#include "StringUtils.h"
#include "LogUtils.h"
#include "IUnknownUtils.h"
#include <netfw.h>

bool CFirewallHelper::AddInboundPrivateTCPRule(const wchar_t* i_pwszRuleName, const wchar_t* i_pwszExecutablePath, const wchar_t* i_pwszLocalPorts)
{
	if (i_pwszRuleName == nullptr)
	{
		LOG("Can't add Firewall Rule: invalid rule name");
		return false;
	}

	if (i_pwszLocalPorts == nullptr)
	{
		i_pwszLocalPorts = L"";	//Enable all local ports
	}

	//Detect executable path in case nullptr is passed to this function.
	std::wstring wsThisExecutablePath;
	if (i_pwszExecutablePath == nullptr)
	{
		wsThisExecutablePath = GetExecutablePath();
		if (wsThisExecutablePath.empty())
		{
			LOG("Can't add Firewall Rule: invalid executable path");
			return false;
		}
		i_pwszExecutablePath = wsThisExecutablePath.data();
	}

	// Initialize COM.
	HRESULT hrComInit = CoInitializeEx(0, COINIT_APARTMENTTHREADED);

	// Ignore RPC_E_CHANGED_MODE; this just means that COM has already been
	// initialized with a different mode. Since we don't care what the mode is,
	// we'll just use the existing mode.
	if (hrComInit != RPC_E_CHANGED_MODE)
	{
		if (FAILED(hrComInit))
		{
			LOG("CoInitializeEx failed: 0x%08lx\n", hrComInit);
			return false;
		}
	}

	INetFwPolicy2* pNetFwPolicy2 = nullptr;
	INetFwRules* pFwRules = nullptr;
	INetFwRule* pFwRule = nullptr;

	BSTR bstrRuleName = SysAllocString(i_pwszRuleName);
	//BSTR bstrRuleDescription = SysAllocString(L"Rule description");
	//BSTR bstrRuleGroup = SysAllocString(L"Rule group");
	BSTR bstrRuleApplication = SysAllocString(i_pwszExecutablePath);   //Application executable path
	BSTR bstrRuleLPorts = SysAllocString(i_pwszLocalPorts);

	// Retrieve INetFwPolicy2
	HRESULT hr = CoCreateInstance(__uuidof(NetFwPolicy2), nullptr, CLSCTX_INPROC_SERVER, __uuidof(INetFwPolicy2), (void**)&pNetFwPolicy2);
	if (FAILED(hr))
	{
		LOG("Can't initialize INetFwPolicy2: 0x%08lx\n", hr);
	}
	else
	{
		// Retrieve INetFwRules
		hr = pNetFwPolicy2->get_Rules(&pFwRules);
		if (FAILED(hr))
		{
			LOG("get_Rules failed: 0x%08lx\n", hr);
		}
		else
		{
			//Avoid Rule duplication: remove previously registered Firewall Rule. Ignore error.
			hr = pFwRules->Remove(bstrRuleName);

			// Create a new Firewall Rule object.
			hr = CoCreateInstance(__uuidof(NetFwRule), nullptr, CLSCTX_INPROC_SERVER, __uuidof(INetFwRule), (void**)&pFwRule);
			if (FAILED(hr))
			{
				LOG("CoCreateInstance for Firewall Rule failed: 0x%08lx\n", hr);
			}
			else
			{
				// Populate the Firewall Rule object
				pFwRule->put_Name(bstrRuleName);
				//pFwRule->put_Description(bstrRuleDescription);
				pFwRule->put_ApplicationName(bstrRuleApplication);
				pFwRule->put_Protocol(NET_FW_IP_PROTOCOL_TCP);
				pFwRule->put_LocalPorts(bstrRuleLPorts);
				pFwRule->put_Direction(NET_FW_RULE_DIR_IN);
				//pFwRule->put_Grouping(bstrRuleGroup);
				pFwRule->put_Profiles(NET_FW_PROFILE2_PRIVATE);
				pFwRule->put_Action(NET_FW_ACTION_ALLOW);
				pFwRule->put_Enabled(VARIANT_TRUE);

				// Add the Firewall Rule
				hr = pFwRules->Add(pFwRule);
				if (FAILED(hr))
				{
					LOG("Firewall Rule Add failed: 0x%08lx\n", hr);
				}
			}
		}
	}

	// Free BSTR's
	SysFreeString(bstrRuleName);
	//SysFreeString(bstrRuleDescription);
	//SysFreeString(bstrRuleGroup);
	SysFreeString(bstrRuleApplication);
	SysFreeString(bstrRuleLPorts);

	// Release the INetFwRule object
	SAFE_RELEASE(pFwRule);
	SAFE_RELEASE(pFwRules);
	SAFE_RELEASE(pNetFwPolicy2);

	// Uninitialize COM.
	if (SUCCEEDED(hrComInit))
	{
		CoUninitialize();
	}

	return SUCCEEDED(hr);
}

bool CFirewallHelper::RemoveRule(const wchar_t* i_pwszRuleName)
{
	if (i_pwszRuleName == nullptr)
	{
		LOG("Can't remove Firewall Rule: invalid rule name");
		return false;
	}

	// Initialize COM.
	HRESULT hrComInit = CoInitializeEx(0, COINIT_APARTMENTTHREADED);

	// Ignore RPC_E_CHANGED_MODE; this just means that COM has already been
	// initialized with a different mode. Since we don't care what the mode is,
	// we'll just use the existing mode.
	if (hrComInit != RPC_E_CHANGED_MODE)
	{
		if (FAILED(hrComInit))
		{
			LOG("CoInitializeEx failed: 0x%08lx\n", hrComInit);
			return false;
		}
	}

	INetFwPolicy2* pNetFwPolicy2 = nullptr;
	INetFwRules* pFwRules = nullptr;

	// Retrieve INetFwPolicy2
	HRESULT hr = CoCreateInstance(__uuidof(NetFwPolicy2), nullptr, CLSCTX_INPROC_SERVER, __uuidof(INetFwPolicy2), (void**)&pNetFwPolicy2);
	if (FAILED(hr))
	{
		LOG("Can't initialize INetFwPolicy2: 0x%08lx\n", hr);
	}
	else
	{
		// Retrieve INetFwRules
		hr = pNetFwPolicy2->get_Rules(&pFwRules);
		if (FAILED(hr))
		{
			LOG("get_Rules failed: 0x%08lx\n", hr);
		}
		else
		{
			BSTR bstrRuleName = SysAllocString(i_pwszRuleName);

			// Remove the Firewall Rule
			hr = pFwRules->Remove(bstrRuleName);
			if (FAILED(hr))
			{
				LOG("Firewall Rule Remove failed: 0x%08lx\n", hr);
			}

			SysFreeString(bstrRuleName);
		}
	}

	SAFE_RELEASE(pFwRules);
	SAFE_RELEASE(pNetFwPolicy2);

	// Uninitialize COM.
	if (SUCCEEDED(hrComInit))
	{
		CoUninitialize();
	}

	return SUCCEEDED(hr);
}
