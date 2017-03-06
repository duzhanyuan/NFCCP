﻿#include "Lsa.hpp"
#include "Util.hpp"

using std::wstring;

unsigned long package_id;

namespace Lsa
{
	PLSA_CREATE_LOGON_SESSION CreateLogonSession;
	PLSA_DELETE_LOGON_SESSION DeleteLogonSession;
	PLSA_ADD_CREDENTIAL AddCredential;
	PLSA_GET_CREDENTIALS GetCredentials;
	PLSA_DELETE_CREDENTIAL DeleteCredential;
	PLSA_ALLOCATE_LSA_HEAP AllocateLsaHeap;
	PLSA_FREE_LSA_HEAP FreeLsaHeap;
	PLSA_ALLOCATE_CLIENT_BUFFER AllocateClientBuffer;
	PLSA_FREE_CLIENT_BUFFER FreeClientBuffer;
	PLSA_COPY_TO_CLIENT_BUFFER CopyToClientBuffer;
	PLSA_COPY_FROM_CLIENT_BUFFER CopyFromClientBuffer;
};

void LoadLsaString(const LSA_STRING * l, wstring & s)
{
	if(!l || !l->Buffer || (l->Length == 0))
	{
		// LSA_STRINGが指定されなかった場合、文字列をクリア
		s.clear();
		return;
	}

	// UCS-2に変換した際のサイズをチェック
	auto newsize = MultiByteToWideChar(CP_UTF7, 0, l->Buffer, l->Length, nullptr, 0);
	if(newsize <= 0)
	{
		// 変換結果が空文字列の場合は、文字列をクリア
		s.clear();
		return;
	}

	auto wstr = new wchar_t[1 + newsize];
	wstr[newsize] = L'\0';
	MultiByteToWideChar(CP_UTF7, 0, l->Buffer, l->Length, wstr, newsize);

	s.assign(wstr);
	delete[] wstr;
}

void * LsaAlloc(size_t s)
{
	return (*Lsa::AllocateLsaHeap)(static_cast<ULONG>(s));
}

void LsaFree(void * p)
{
	(*Lsa::FreeLsaHeap)(p);
}

extern "C" NTSTATUS NTAPI LsaApCallPackage(
	// arguments
	PLSA_CLIENT_REQUEST ClientRequest,
	void *ProtocolSubmitBuffer,
	void *ClientBufferBase,
	unsigned long SubmitBufferLength,
	// returns
	void **ProtocolReturnBuffer,
	unsigned long *ReturnBufferLength,
	NTSTATUS *ProtocolStatus)
{
	DebugPrint(L"%hs", "LsaApCallPackage");
	return STATUS_NOT_IMPLEMENTED;
}

extern "C" NTSTATUS NTAPI LsaApCallPackagePassthrough(
	// arguments
	PLSA_CLIENT_REQUEST ClientRequest,
	void * ProtocolSubmitBuffer,
	void * ClientBufferBase,
	unsigned long SubmitBufferLength,
	// returns
	void ** ProtocolReturnBuffer,
	unsigned long * ReturnBufferLength,
	NTSTATUS * ProtocolStatus)
{
	DebugPrint(L"%hs", "LsaApCallPackagePassthrough");
	return STATUS_NOT_IMPLEMENTED;
}

extern "C" NTSTATUS NTAPI LsaApCallPackageUntrusted(
	// arguments
	PLSA_CLIENT_REQUEST ClientRequest,
	void * ProtocolSubmitBuffer,
	void * ClientBufferBase,
	unsigned long SubmitBufferLength,
	// returns
	void ** ProtocolReturnBuffer,
	unsigned long * ReturnBufferLength,
	NTSTATUS * ProtocolStatus)
{
	DebugPrint(L"%hs", "LsaApCallPackageUntrusted");
	return STATUS_NOT_IMPLEMENTED;
}

extern "C" NTSTATUS NTAPI LsaApInitializePackage(
	// arguments
	unsigned long AuthenticationPackageId,
	LSA_DISPATCH_TABLE * LsaDispatchTable,
	LSA_STRING * Database,
	LSA_STRING * Confidentiality,
	// returns
	LSA_STRING ** AuthenticationPackageName)
{
	DebugPrint(L"%hs", "LsaApInitializePackage");

	// LSAの関数
	Lsa::CreateLogonSession = LsaDispatchTable->CreateLogonSession;
	Lsa::DeleteLogonSession = LsaDispatchTable->DeleteLogonSession;
	Lsa::AddCredential = LsaDispatchTable->AddCredential;
	Lsa::GetCredentials = LsaDispatchTable->GetCredentials;
	Lsa::DeleteCredential = LsaDispatchTable->DeleteCredential;
	Lsa::AllocateLsaHeap = LsaDispatchTable->AllocateLsaHeap;
	Lsa::FreeLsaHeap = LsaDispatchTable->FreeLsaHeap;
	Lsa::AllocateClientBuffer = LsaDispatchTable->AllocateClientBuffer;
	Lsa::FreeClientBuffer = LsaDispatchTable->FreeClientBuffer;
	Lsa::CopyToClientBuffer = LsaDispatchTable->CopyToClientBuffer;
	Lsa::CopyFromClientBuffer = LsaDispatchTable->CopyFromClientBuffer;

	package_id = AuthenticationPackageId;
	DebugPrint(L"Authentication package ID:%u", package_id);

	// データベース名と守秘義務？は未使用のため意味なし…
	LoadLsaString(Database, *database);
	LoadLsaString(Confidentiality, *confidentiality);

	DebugPrint(L"Database:%s", database->c_str());
	DebugPrint(L"Confidentiality:%s", confidentiality->c_str());

	auto package_name = ustring(L"nfcidauth");
	package_name.set_allocater(LsaAlloc, LsaFree);
	*AuthenticationPackageName = package_name.to_lsa_string();

	return STATUS_NOT_IMPLEMENTED;
}

extern "C" void NTAPI LsaApLogonTerminated(LUID * LogonId)
{
	DebugPrint(L"%hs", "LsaApLogonTerminated");
}

/*
 * ログオン時に呼び出される処理
 * ProtocolSubmitBufferはLsaLogonUser関数のAuthenticationInformation引数で指定されるものと同じ
 *
 * LogonId、ProfileBuffer、トークン情報を返す
 * LogonIdはランダムで生成されたID(その実体は符号付き64bit整数)
 * ProfileBufferはログオン結果(ユーザー名など)を呼び出し元に伝えるもの
 * トークン情報はユーザーとグループのSIDをはじめとし、特権、DACLを指定したもの
 * Exはコンピューター名を、Ex2はさらに追加の資格情報を返す
 */

extern "C" NTSTATUS NTAPI LsaApLogonUser(
	// arguments
	PLSA_CLIENT_REQUEST ClientRequest,
	SECURITY_LOGON_TYPE LogonType,
	void * ProtocolSubmitBuffer,
	void * ClientBufferBase,
	unsigned long SubmitBufferSize,
	// returns
	void ** ProfileBuffer,
	unsigned long * ProfileBufferSize,
	LUID * LogonId,
	NTSTATUS * SubStatus,
	LSA_TOKEN_INFORMATION_TYPE * TokenInformationType,
	void ** TokenInformation,
	UNICODE_STRING ** AccountName,
	UNICODE_STRING ** AuthenticatingAuthority)
{
	DebugPrint(L"%hs", "LsaApLogonUser");
	return STATUS_NOT_IMPLEMENTED;
}

extern "C" NTSTATUS NTAPI LsaApLogonUserEx(
	// arguments
	PLSA_CLIENT_REQUEST ClientRequest,
	SECURITY_LOGON_TYPE LogonType,
	void * ProtocolSubmitBuffer,
	void * ClientBufferBase,
	unsigned long SubmitBufferSize,
	// returns
	void ** ProfileBuffer,
	unsigned long * ProfileBufferSize,
	LUID * LogonId,
	NTSTATUS * SubStatus,
	LSA_TOKEN_INFORMATION_TYPE * TokenInformationType,
	void ** TokenInformation,
	UNICODE_STRING ** AccountName,
	UNICODE_STRING ** AuthenticatingAuthority,
	UNICODE_STRING ** MachineName)
{
	DebugPrint(L"%hs", "LsaApLogonUserEx");
	return STATUS_NOT_IMPLEMENTED;
}

extern "C" NTSTATUS NTAPI LsaApLogonUserEx2(
	// arguments
	PLSA_CLIENT_REQUEST ClientRequest,
	SECURITY_LOGON_TYPE LogonType,
	void * ProtocolSubmitBuffer,
	void * ClientBufferBase,
	unsigned long SubmitBufferSize,
	// returns
	void ** ProfileBuffer,
	unsigned long * ProfileBufferSize,
	LUID * LogonId,
	NTSTATUS * SubStatus,
	LSA_TOKEN_INFORMATION_TYPE * TokenInformationType,
	void ** TokenInformation,
	UNICODE_STRING ** AccountName,
	UNICODE_STRING ** AuthenticatingAuthority,
	UNICODE_STRING ** MachineName,
	SECPKG_PRIMARY_CRED * PrimaryCredentials,
	SECPKG_SUPPLEMENTAL_CRED_ARRAY ** SupplementalCredentials)
{
	DebugPrint(L"%hs", "LsaApLogonUserEx2");
	return STATUS_NOT_IMPLEMENTED;
}
