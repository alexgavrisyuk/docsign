/*********************************************************************

 ADOBE SYSTEMS INCORPORATED
 Copyright (C) 1998-2006 Adobe Systems Incorporated
 All rights reserved.

 NOTICE: Adobe permits you to use, modify, and distribute this file
 in accordance with the terms of the Adobe license agreement
 accompanying it. If you have received this file from a source other
 than Adobe, then your use, modification, or distribution of it
 requires the prior written permission of Adobe.

 ---------------------------------------------------------------------

 DocSignInit.cpp

*********************************************************************/

#include "DocSign.h"
#include "DSHandler.h"
#include "FormsHFT.h"

DSHandler *gpDSHandler;

HFT gPubSecHFT = NULL;
HFT gDigSigHFT = NULL;
HFT gAcroFormHFT = NULL;

/*-------------------------------------------------------
	Core Handshake Callbacks
-------------------------------------------------------*/

// stuff for Menu set up 
static AVMenuItem menuItem = NULL;
ACCB1 ASBool ACCB2 PluginMenuItem(char* MyMenuItemTitle, char* MyMenuItemName);

// callback functions implemented in file "BasicPlugin.cpp"
extern ACCB1 void ACCB2 MyPluginCommand(void* clientData);
extern ACCB1 ASBool ACCB2 MyPluginIsEnabled(void* clientData);
extern ACCB1 ASBool ACCB2 MyPluginSetmenu();

extern const char* MyPluginExtensionName;



/* DocSignExportHFTs
** ------------------------------------------------------
**
** Create and register the HFT's.
**
** Return true to continue loading plug-in.
** Return false to cause plug-in loading to stop.
*/
ACCB1 ASBool ACCB2 DocSignExportHFTs(void)
{
	return true;
}

/* DocSignImportReplaceAndRegister
** ------------------------------------------------------
**
** This routine is where you can:
**	1) Import plug-in supplied HFTs.
**	2) Replace functions in the HFTs you're using (where allowed).
**	3) Register to receive notification events.
**
** Return true to continue loading plug-in.
** Return false to cause plug-in loading to stop.
*/
ACCB1 ASBool ACCB2 DocSignImportReplaceAndRegister(void)
{
	/* PubSec HFT */
	gPubSecHFT = ASExtensionMgrGetHFT(ASAtomFromString("PubSecHFT"), 1);
	if (!gPubSecHFT)
		return false;

	gDigSigHFT = ASExtensionMgrGetHFT(ASAtomFromString("DigSigHFT"), 1);
	if (!gDigSigHFT)
		return false;

	gAcroFormHFT = Init_AcroFormHFT;
	if(!gAcroFormHFT)
		return false;

	return true;
}

/**
	The main initialization routine.
	We register our action handler with the application.
	@return true to continue loading the plug-in
	@return false to cause plug-in loading to stop.
*/
/* PluginInit
** ------------------------------------------------------
**
** The main initialization routine.
**
** Return true to continue loading plug-in.
** Return false to cause plug-in loading to stop.
*/
ACCB1 ASBool ACCB2 PluginInit(void)
{
	return MyPluginSetmenu();
}

/**
	A convenient function to add a menu item under Acrobat SDK menu.
	@param MyMenuItemTitle IN String for the menu item's title.
	@param MyMenuItemName IN String for the menu item's internal name.
	@return true if successful, false if failed.
	@see AVAppGetMenubar
	@see AVMenuItemNew
	@see AVMenuItemSetExecuteProc
	@see AVMenuItemSetComputeEnabledProc
	@see AVMenubarAcquireMenuItemByName
	@see AVMenubarAcquireMenuByName
*/
ACCB1 ASBool ACCB2 PluginMenuItem(char* MyMenuItemTitle, char* MyMenuItemName)
{
	AVMenubar menubar = AVAppGetMenubar();
	AVMenu volatile commonMenu = NULL;

	if (!menubar)
		return false;

	DURING

		// Create our menuitem
		menuItem = AVMenuItemNew(MyMenuItemTitle, MyMenuItemName, NULL, true, NO_SHORTCUT, 0, NULL, gExtensionID);
	AVMenuItemSetExecuteProc(menuItem, ASCallbackCreateProto(AVExecuteProc, MyPluginCommand), NULL);
	AVMenuItemSetComputeEnabledProc(menuItem,
		ASCallbackCreateProto(AVComputeEnabledProc, MyPluginIsEnabled), (void*)pdPermEdit);

	commonMenu = AVMenubarAcquireMenuByName(menubar, "ADBE:Acrobat_SDK");
	// if "Acrobat SDK" menu is not existing, create one.
	if (!commonMenu) {
		commonMenu = AVMenuNew("Acrobat SDK", "ADBE:Acrobat_SDK", gExtensionID);
		AVMenubarAddMenu(menubar, commonMenu, APPEND_MENU);
	}

	AVMenuAddMenuItem(commonMenu, menuItem, APPEND_MENUITEM);

	AVMenuRelease(commonMenu);

	HANDLER
		if (commonMenu)
			AVMenuRelease(commonMenu);
	return false;
	END_HANDLER

		return true;
}


/* DocSignInit
** ------------------------------------------------------
**
** The main initialization routine.
**
** Return true to continue loading plug-in.
** Return false to cause plug-in loading to stop.
*/
ACCB1 ASBool ACCB2 DocSignInit(void)
{
	PluginInit();

	gpDSHandler = new DSHandler();
	if (gpDSHandler != NULL)
		return gpDSHandler->Register();

	return false;
}


/* DocSignUnload
** ------------------------------------------------------
**
** The unload routine.
**
** Called when your plug-in is being unloaded when the application quits.
** Use this routine to release any system resources you may have
** allocated.
**
** Returning false will cause an alert to display that unloading failed.
*/
ACCB1 ASBool ACCB2 DocSignUnload(void)
{
	if (gpDSHandler != NULL) {
		gpDSHandler->Unregister();
		delete gpDSHandler;
	}

	return true;
}

/* GetExtensionName
** ------------------------------------------------------
**
** Get the extension name associated with this plugin
*/
ASAtom GetExtensionName()
{
	return ASAtomFromString("ADBE:DocSign");	/* Change to your extension's name */
}


/*
** PIHandshake
** Required Plug-in handshaking routine: Do not change it's name! PIMain.c expects
** this function to be named and typed as shown.
*/
ACCB1 ASBool ACCB2 PIHandshake(Uns32 handshakeVersion, void *handshakeData)
{
	if (handshakeVersion == HANDSHAKE_V0200) {
		/* Cast handshakeData to the appropriate type */
		PIHandshakeData_V0200 *hsData = (PIHandshakeData_V0200 *)handshakeData;

		/* Set the name we want to go by */
		hsData->extensionName = GetExtensionName();

		/* If you export your own HFT, do so in here */
		hsData->exportHFTsCallback = ASCallbackCreate(&DocSignExportHFTs);

		/*
		** If you import plug-in HFTs, replace functionality, and/or want to register for notifications before
		** the user has a chance to do anything, do so in here.
		*/
		hsData->importReplaceAndRegisterCallback = ASCallbackCreate(
																		 &DocSignImportReplaceAndRegister);

		/* Perform your plug-in's initialization in here */
		hsData->initCallback = ASCallbackCreate(&DocSignInit);

		/* Perform any memory freeing or state saving on "quit" in here */
		hsData->unloadCallback = ASCallbackCreate(&DocSignUnload);

		/* All done */
		return true;

	} /* Each time the handshake version changes, add a new "else if" branch */

	/*
	** If we reach here, then we were passed a handshake version number we don't know about.
	** This shouldn't ever happen since our main() routine chose the version number.
	*/
	return false;
}
