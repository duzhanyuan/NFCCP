﻿#include "Provider.hpp"
#include "Util.hpp"
#include <ShlGuid.h>
#include <Shlwapi.h>

const CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR CCredentialProvider::_fields[] = {
	{0, CREDENTIAL_PROVIDER_FIELD_TYPE::CPFT_TILE_IMAGE, nullptr, CPFG_CREDENTIAL_PROVIDER_LOGO},
	{1, CREDENTIAL_PROVIDER_FIELD_TYPE::CPFT_LARGE_TEXT, L"ICカード", CPFG_CREDENTIAL_PROVIDER_LABEL},
	{2, CREDENTIAL_PROVIDER_FIELD_TYPE::CPFT_SMALL_TEXT, L"カードをセットしてログオン", GUID_NULL}};

const FIELD_STATE_PAIR CCredentialProvider::field_states[] = {
	{CREDENTIAL_PROVIDER_FIELD_STATE::CPFS_DISPLAY_IN_BOTH, CREDENTIAL_PROVIDER_FIELD_INTERACTIVE_STATE::CPFIS_NONE},
	{CREDENTIAL_PROVIDER_FIELD_STATE::CPFS_DISPLAY_IN_BOTH, CREDENTIAL_PROVIDER_FIELD_INTERACTIVE_STATE::CPFIS_NONE},
	{CREDENTIAL_PROVIDER_FIELD_STATE::CPFS_DISPLAY_IN_SELECTED_TILE, CREDENTIAL_PROVIDER_FIELD_INTERACTIVE_STATE::CPFIS_NONE}};

CCredentialProvider::CCredentialProvider()
{
	this->instances = 0UL;
	this->AddRef();

	this->fields = new CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR[2];
	for(int i = 0; i < 3; i++)
	{
		this->fields[i] = _fields[i];
	}

	this->renew_credentials = true;
	this->credential = nullptr;
}

CCredentialProvider::~CCredentialProvider()
{
	if(this->credential)
	{
		this->credential->Release();
		this->credential = nullptr;
	}
	delete this->fields;
}

HRESULT CCredentialProvider::QueryInterface(REFIID riid, void ** ppvObject)
{
#pragma warning(push)
#pragma warning(disable:4838)
	static const QITAB interfaces[] = {
		QITABENT(CCredentialProvider, ICredentialProvider),
		{}
	};
#pragma warning(pop)
	return QISearch(this, interfaces, riid, ppvObject);
}

ULONG CCredentialProvider::AddRef()
{
	_InterlockedIncrement(&global_instances);
	return _InterlockedIncrement(&this->instances);
}

ULONG CCredentialProvider::Release()
{
	auto decr = _InterlockedDecrement(&this->instances);
	if(decr == 0UL)
	{
		delete this;
	}
	_InterlockedDecrement(&global_instances);
	return decr;
}

HRESULT CCredentialProvider::SetUsageScenario(CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus, DWORD dwFlags)
{

	switch(cpus)
	{
	case CREDENTIAL_PROVIDER_USAGE_SCENARIO::CPUS_LOGON:
	case CREDENTIAL_PROVIDER_USAGE_SCENARIO::CPUS_UNLOCK_WORKSTATION:
	case CREDENTIAL_PROVIDER_USAGE_SCENARIO::CPUS_CREDUI:
		this->usage_scenario = cpus;
		this->valid_scenario = true;
		this->renew_credentials = true;
		return S_OK;

	case CREDENTIAL_PROVIDER_USAGE_SCENARIO::CPUS_CHANGE_PASSWORD:
		this->valid_scenario = false;
		return E_NOTIMPL;

	default:
		this->valid_scenario = false;
		return E_INVALIDARG;
	}
}

HRESULT CCredentialProvider::SetSerialization(const CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION * pcpcs)
{
	return E_NOTIMPL;
}

HRESULT CCredentialProvider::Advise(ICredentialProviderEvents *, UINT_PTR)
{
	return E_NOTIMPL;
}

HRESULT CCredentialProvider::UnAdvise()
{
	return E_NOTIMPL;
}

HRESULT CCredentialProvider::GetFieldDescriptorCount(DWORD *pdwCount)
{
	*pdwCount = 3;
	return S_OK;
}

HRESULT CCredentialProvider::GetFieldDescriptorAt(DWORD dwIndex, CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR ** ppcpfd)
{
	if(dwIndex >= 3)
	{
		return E_INVALIDARG;
	}

	auto desc = static_cast<CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR*>(CoTaskMemAlloc(sizeof(CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR)));
	*desc = this->fields[dwIndex];

	if(this->fields[dwIndex].pszLabel)
	{
		SHStrDupW(this->fields[dwIndex].pszLabel, &desc->pszLabel);
	}

	*ppcpfd = desc;

	return S_OK;
}

HRESULT CCredentialProvider::GetCredentialCount(DWORD * pdwCount, DWORD * pdwDefault, BOOL * pbAutoLogonWithDefault)
{
	if(this->renew_credentials)
	{
		this->renew_credentials = false;
		if(this->credential)
		{
			this->credential->Release();
			this->credential = nullptr;
		}
		if(this->valid_scenario)
		{
			this->credential = new CCredentialProviderCredential();
		}
	}

	*pdwCount = 1;
	*pdwDefault = CREDENTIAL_PROVIDER_NO_DEFAULT;
	*pbAutoLogonWithDefault = FALSE;

	return S_OK;
}

HRESULT CCredentialProvider::GetCredentialAt(DWORD dwIndex, ICredentialProviderCredential ** ppcpc)
{
	if(dwIndex != 0)
	{
		return E_INVALIDARG;
	}

	return this->credential->QueryInterface(__uuidof(ICredentialProviderCredential), (void **)ppcpc);
}