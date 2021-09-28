//
//
//

#include "frontend.h"
//#include "../Manager.h"
//#include "../config/config.h"

#include <stdio.h>
#include <string.h>
#include <string>

std::string extract_param(const char *pcParam) {
	int iLen = strlen(pcParam); if (iLen > 63) iLen = 63;
	std::string s;
	for(int i = 0; i < iLen; i++) {
		if (pcParam[i] == '/') break;
		s.push_back(pcParam[i]);
	}
	return s;
}

int CFrontend::handleRequest(const char *pszCommand, Json::Value &in, Json::Value &out)
{
	int iRes = FRONTEND_OK;
	if (strcmp(pszCommand, "all") == 0) {
		iRes = 0; //(m_pParent->listConnectedMachines(out))? FRONTEND_OK : FRONTEND_ERROR;
	} else
	if (strstr(pszCommand, "test/") == pszCommand) {
		// let's decode serial number
		std::string s = extract_param(pszCommand + 5);
		if (s.size() > 0) {
			iRes = 0; //(m_pParent->performTest(s.c_str()))? FRONTEND_OK : FRONTEND_ERROR;
			out["success"] = ((iRes == FRONTEND_OK)? true : false);
		} else iRes = FRONTEND_NOT_FOUND;
	} else
	if (strstr(pszCommand, "bill123/") == pszCommand) {
		out = in;
		out["blah"] = "blahblahcar";
	} else
	if (strstr(pszCommand, "bill/") == pszCommand) {
		// let's decode INN
		std::string sInn = extract_param(pszCommand + 5);
		if (sInn.size() > 0) {
			// let's decode serial number
			int iOffset = 5 + sInn.size() + 1;
			std::string s = (strlen(pszCommand) <= iOffset)? "" : extract_param(pszCommand + iOffset);

			iRes = 0; //(m_pParent->processBill(sInn.c_str(), (s.size() == 0)? 0 : s.c_str(), in, out))? FRONTEND_OK : FRONTEND_ERROR;
			out["success"] = ((iRes == FRONTEND_OK)? true : false);
		} else iRes = FRONTEND_NOT_FOUND;
	} else
	if (strcmp(pszCommand, "config") == 0) {
//		out = CConfig::get_instance()->get_config();
		iRes = FRONTEND_OK;
	} else {

		iRes = FRONTEND_NOT_FOUND;
	}

	return iRes;
}
