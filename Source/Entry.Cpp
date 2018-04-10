#ifndef _ENTRY_CPP
#define _ENTRY_CPP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <Stdio.H>
#include <Stdlib.H>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "../Resources/Resource.H"

#include "Entry.H"
#include "Init.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define BUFFERSIZE 75

HINSTANCE ghAppInstance = NULL;

char gsAppCopyRight[64];
char gsFileVersion[32];
char gsTitleCaption[64];

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct _tag_Configuration_Info{
	char sReplace[1024];
	char sWith[1024];

	bool bIsCCReplace[1024];

	int iReplaceLen;
	int iWithLen;

	int iMatchPos;

	char sSourceFile[MAX_PATH];
	char sTargetFile[MAX_PATH];
	bool bSilent;
	bool bIgnoreCase;
} CONFIGINFO, *LPCONFIGINFO;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

__inline bool IsMatch(CONFIGINFO *pCI, const char *sBuf)
{
	for(pCI->iMatchPos = 0; pCI->iMatchPos < pCI->iReplaceLen; pCI->iMatchPos++)
	{
		if(pCI->bIsCCReplace && pCI->sReplace[pCI->iMatchPos] == '?')
		{
			//Skip this char, its a match.
		}
		else{
			if(pCI->bIgnoreCase)
			{
				if(tolower(sBuf[pCI->iMatchPos]) != tolower(pCI->sReplace[pCI->iMatchPos]))
				{
					return false;
				}
			}
			else{
				if(sBuf[pCI->iMatchPos] != pCI->sReplace[pCI->iMatchPos])
				{
					return false;
				}
			}
		}
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ProcessFile(CONFIGINFO *pCI)
{
	bool bResult = true;
	FILE *hSource = NULL;
	FILE *hTarget = NULL;
	char *sBuf = NULL;
	int iChar = 0;

	int iAlloc = pCI->iReplaceLen + 1;

	if(fopen_s(&hSource, pCI->sSourceFile, "rb") != 0)
	{
		printf("Failed to open \"%s\".\n", pCI->sSourceFile);
		return false;
	}
	if(fopen_s(&hTarget, pCI->sTargetFile, "wb") != 0)
	{
		fclose(hSource);
		printf("Failed to create \"%s\".\n", pCI->sTargetFile);
		return false;
	}

	if(!(sBuf = (char *) calloc(iAlloc + 1, sizeof(char))))
	{
		printf("Failed to allocate %d bytes of RAM.\n", BUFFERSIZE * sizeof(char));
		CloseHandle(hSource);
		return false;
	}

	int iAvailPos = 0;
	int iRePos = 0;

	while((iChar = fgetc(hSource)) != EOF)
	{
		if(iAvailPos < iAlloc)
		{
			sBuf[iAvailPos++] = iChar; // Fill the buffer.
		}
		else{
			iRePos = 0;

			if(IsMatch(pCI, sBuf))
			{
				if(pCI->iWithLen)
				{
					fwrite(pCI->sWith, sizeof(char), pCI->iWithLen, hTarget);
				}
				iAvailPos = 0; //Flush the buffer.
				sBuf[iAvailPos++] = sBuf[iAlloc - 1]; //Save the last char.
				sBuf[iAvailPos++] = iChar; //Save the last read char.
			}
			else{
				fputc(sBuf[0], hTarget);
				for(iRePos = 0; iRePos < iAlloc - 1; iRePos++)
				{
					sBuf[iRePos] = sBuf[iRePos + 1];
				}
				sBuf[iRePos] = iChar;
			}
		}
	}

	fwrite(sBuf, sizeof(char), iAvailPos, hTarget);

	fclose(hTarget);
	fclose(hSource);

	free(sBuf);

	return bResult;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int IsCmd(const char *sArgument, const char *sCommand)
{
	int iCommandLen = strlen(sCommand);
	if(_strnicmp(sArgument, sCommand, iCommandLen) == 0)
	{
		return iCommandLen;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int GetFlagedValue(const char *sBuf, int iSkpLen, int iBufSz, char *sOutBuf)
{
	int iWPos = 0;
	int iRPos = iSkpLen;

	while(iRPos < iBufSz)
	{
		sOutBuf[iWPos++] = sBuf[iRPos++];
	}

	sOutBuf[iWPos] = '\0';

	return iWPos; //The length of the string returned in sOutBuf.
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void WriteHelp(void)
{
	printf("\n%s - %s\n", gsTitleCaption, gsFileVersion);
	printf("%s\n\n", gsAppCopyRight);

	printf("Syntax:\n");
	printf(" FindReplace [/Source:] [/Target:] [/Replace:] [/With:] [/IgnoreCase]\n");
	printf("  /?          - Displays this help message.\n");
	printf("  /Source:    - Defines the source file that will searched.\n");
	printf("  /Target:    - Defines the target file will be have values replaced.\n");
	printf("  /Replace:   - Defines the text to search for, see special characters below.\n");
	printf("  /With:      - Defines the text that will replace any found occurrence.\n");
	printf("  /IgnoreCase - Tells the application to ignore CASE on printable characters.\n");
	printf("\n");

	printf(" The replace string can contain the following:\n");
	printf("   ? - Match any character\n");
	printf("  \\r - Carriage Return character\n");
	printf("  \\n - Line Feed character\n");
	printf("  \\t - Tab character\n");
	printf("  \\p - Pipe character\n");
	printf("  \\0 - Null Character\n");
	printf("  \\? - Literal Question Mark\n");
	printf("  \\s - Literal Single Quote\n");
	printf("  \\q - Literal Double Quote\n");
	printf("  \\\\ - Single Backslash character\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
	Returns the mask length;

	CCMask is the "Control Character" mask.
*/

int LoadMask(const char *sSource, int iSourceLen, char *sTarget, int iMaxTargetSz, bool *bCCMask)
{
	int iWPos = 0;

	for(int iRPos = 0; iRPos < iSourceLen; iRPos++)
	{
		if(sSource[iRPos] == '\\')
		{
			iRPos++;

			if(_strnicmp(sSource + iRPos, "R", 1) == 0)
			{
				sTarget[iWPos++] = '\r';
			}
			else if(_strnicmp(sSource + iRPos, "N", 1) == 0)
			{
				sTarget[iWPos++] = '\n';
			}
			else if(_strnicmp(sSource + iRPos, "T", 1) == 0)
			{
				sTarget[iWPos++] = '\t';
			}
			else if(_strnicmp(sSource + iRPos, "P", 1) == 0)
			{
				sTarget[iWPos++] = '|';
			}
			else if(_strnicmp(sSource + iRPos, "?", 1) == 0)
			{
				sTarget[iWPos++] = '?';
			}
			else if(_strnicmp(sSource + iRPos, "s", 1) == 0)
			{
				sTarget[iWPos++] = '\'';
			}
			else if(_strnicmp(sSource + iRPos, "q", 1) == 0)
			{
				sTarget[iWPos++] = '\"';
			}
			else if(_strnicmp(sSource + iRPos, "0", 1) == 0)
			{
				sTarget[iWPos++] = '\0';
			}
			else if(_strnicmp(sSource + iRPos, "\\", 1) == 0)
			{
				sTarget[iWPos++] = '\\';
			}
			else{
				printf("Unknown control character.\n");
				return 0;
			}
		}
		else{

			if(bCCMask)
			{
				if(sSource[iRPos] == '?')	
				{
					bCCMask[iWPos] = true;
				}
				else{
					bCCMask[iWPos] = false;
				}
			}
	
			sTarget[iWPos] = sSource[iRPos];

			iWPos++;
		}
	}

	return iWPos;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ParseCmdLine(CONFIGINFO *pCI, int iArgs, char *sArgs[])
{
	int iArgSz = 0;

	if(iArgs <= 1)
	{
		WriteHelp();
		return false;
	}

	for(int iArg = 1; iArg < iArgs; iArg++)
	{
		if(IsCmd(sArgs[iArg], "/?") || IsCmd(sArgs[iArg], "-?"))
		{
			WriteHelp();
			return false;
		}
		else if(IsCmd(sArgs[iArg], "/Silent"))
		{
			pCI->bSilent = true;
		}
		else if(IsCmd(sArgs[iArg], "/IgnoreCase"))
		{
			pCI->bIgnoreCase = true;
		}
		else if((iArgSz = IsCmd(sArgs[iArg], "/Source:")))
		{
			GetFlagedValue(sArgs[iArg], iArgSz, strlen(sArgs[iArg]), pCI->sSourceFile);
		}
		else if((iArgSz = IsCmd(sArgs[iArg], "/Target:")))
		{
			GetFlagedValue(sArgs[iArg], iArgSz, strlen(sArgs[iArg]), pCI->sTargetFile);
		}
		else if((iArgSz = IsCmd(sArgs[iArg], "/Replace:")))
		{
			char sReplace[sizeof(pCI->sReplace)];
			GetFlagedValue(sArgs[iArg], iArgSz, strlen(sArgs[iArg]), sReplace);
			if((pCI->iReplaceLen = LoadMask(sReplace, strlen(sReplace),
				pCI->sReplace, sizeof(pCI->sReplace), pCI->bIsCCReplace)) == 0)

			{
				printf("The replace mask is invalid.\n");
				return false;
			}
		}
		else if((iArgSz = IsCmd(sArgs[iArg], "/With:")))
		{
			char sWith[sizeof(pCI->sWith)];
			GetFlagedValue(sArgs[iArg], iArgSz, strlen(sArgs[iArg]), sWith);
			if((pCI->iWithLen = LoadMask(sWith, strlen(sWith),
				pCI->sWith, sizeof(pCI->sWith), NULL)) == 0)
			{
				printf("The replace with mask is invalid. Omit this parameter to replace with nothing.\n");
				return false;
			}
		}
		else{
			printf("Unknown argument: [%s]\n", sArgs[iArg]);
			return false;
		}
	}

	if(strlen(pCI->sTargetFile) == 0)
	{
		printf("No target file was specified.\n");
		return false;
	}
	else if(pCI->sSourceFile == 0)
	{
		printf("No source file was specified.\n");
		return false;
	}
	else if(pCI->iReplaceLen == 0)
	{
		printf("No replace text was specified.\n");
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int iArg, char *sArg[])
{
	if(!InitializeApp())
    {
		printf("Failed to initialize the appication.");
		return 1;
    }

	CONFIGINFO CI;
	memset(&CI, 0, sizeof(CONFIGINFO));

	int iResult = 1;

	if(ParseCmdLine(&CI, iArg, sArg))
	{
		iResult = !ProcessFile(&CI);
	}

	if(!UninitializeApp())
    {
        return 1;
    }

	return iResult;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
