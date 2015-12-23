//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************
// TSRoute.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"
#include "tsroute.h"
#include "tssvc.h"

bool InstallService(const char* xmlfile) 
{ 
  
    char strDir[1024]; 
    char strPath[1024]; 
    SC_HANDLE schSCManager,schService; 
  
	GetModuleFileName(NULL,strPath, sizeof(strPath));
    GetCurrentDirectory(sizeof(strDir),strDir);
	ostringstream oss;
	oss << "\"" << strPath << "\"";
	if((xmlfile) && (strlen(xmlfile)>0))
	{
		string s = "";
		
		GetFullPath(s, xmlfile);

		if(s.length()>0)
			oss << " -service -stream -xmlfile \"" << s.c_str() << "\"";
	}
	else
	    oss << " -service -stream ";


    schSCManager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS); 
  
	if (schSCManager == NULL)
	{		
		DWORD dwError = GetLastError();
		return false;
	}
    
  
    schService = CreateService(schSCManager,SZSERVICENAME,"MPEG2-TS Streamer", 
            SERVICE_ALL_ACCESS,         // desired access
            SERVICE_WIN32_OWN_PROCESS,  // service type
            SERVICE_AUTO_START,       // start type
            SERVICE_ERROR_NORMAL,       // error control type
		oss.str().c_str(), // service's binary 
        NULL, // no load ordering group 
        NULL, // no tag identifier 
        NULL, // no dependencies 
        NULL, // Si null demarrer en tant que compte system 
        NULL); // Mot de passe : null si demarrer en tant que system 
  
    if (schService == NULL) 
	{
		DWORD dwError =  GetLastError() ;
		return false;
	}

	SERVICE_DESCRIPTION sd;
	sd.lpDescription = "This service streams Single Program (SPTS) or Multiple Program (MPTS) MPEG2-TS files towards the AServer";
	ChangeServiceConfig2(schService,SERVICE_CONFIG_DESCRIPTION,&sd);

    CloseServiceHandle(schService); 
    return true; 
} 
  
  
bool UninstallService() 
{ 
    SC_HANDLE schSCManager; 
    SC_HANDLE hService; 
  
    schSCManager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS); 
  
    if (schSCManager == NULL) 
        return false; 
  
    hService=OpenService(schSCManager,SZSERVICENAME,SERVICE_ALL_ACCESS); 
  
    if (hService == NULL) 
        return false; 
  
    if(DeleteService(hService)==0) 
        return false; 
  
    if(CloseServiceHandle(hService)==0) 
        return false; 
    else 
        return true; 
} 
bool StartService() 
{ 
    SC_HANDLE schSCManager; 
    SC_HANDLE hService; 
  
    schSCManager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS); 
  
    if (schSCManager == NULL) 
        return false; 
  
    hService=OpenService(schSCManager,SZSERVICENAME,SERVICE_ALL_ACCESS); 
  
    if (hService == NULL) 
        return false; 
  
    if(StartService(hService,0,NULL)==0) 
        return false; 
  
    if(CloseServiceHandle(hService)==0) 
        return false; 
    else 
        return true; 
} 
bool StopService() 
{ 
    SC_HANDLE schSCManager; 
    SC_HANDLE hService; 
  
    schSCManager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS); 
  
    if (schSCManager == NULL) 
        return false; 
  
    hService=OpenService(schSCManager,SZSERVICENAME,SERVICE_ALL_ACCESS); 
  
    if (hService == NULL) 
        return false; 
  
	SERVICE_STATUS status;
    if(ControlService(hService,SERVICE_CONTROL_STOP, &status)==0) 
		return false; 
  
    if(CloseServiceHandle(hService)==0) 
        return false; 
    else 
        return true; 
} 


