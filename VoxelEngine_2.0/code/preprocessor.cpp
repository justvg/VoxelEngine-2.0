#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <vector>
#include <string>
#include <algorithm>

static char *
ReadEntireFileAndNullTerminate(char *Filename)
{
    char *Result = 0;

    FILE *File = fopen(Filename, "rb");
    if(File)
    {
        fseek(File, 0, SEEK_END);
        size_t FileSize = ftell(File);
        fseek(File, 0, SEEK_SET);

        Result = (char *)malloc(FileSize + 1);
        size_t BytesRead = fread(Result, FileSize, 1, File);
        Result[FileSize] = 0;

        fclose(File);
    }   

    return(Result);
}

struct tokenizer
{
    char *At;
};

enum token_type
{
    Token_Unknown,

    Token_Comma,
    Token_OpenParen,
    Token_CloseParen,
    Token_Colon,
    Token_Semicolon,
    Token_Asterisk,
    Token_OpenBracket,
    Token_CloseBracket,
    Token_OpenBrace,
    Token_CloseBrace,

    Token_String,
    Token_Identifier,

    Token_EndOfFile
};

struct token
{
    token_type Type;

    size_t StringLength;
    char *String;
};

static bool
IsAlpha(char C)
{
    bool Result = ((C >= 'a') && (C <= 'z')) ||
                  ((C >= 'A') && (C <= 'Z'));

    return(Result);
}

static bool
IsNumber(char C)
{
    bool Result = ((C >= '0') && (C <= '9'));

    return(Result);
}

static bool
IsWhitespace(char C)
{
    bool Result = ((C == ' ') ||
                   (C == '\t') ||
                   (C == '\n') ||
                   (C == '\r'));

    return(Result);
}

static bool
IsEndOfLine(char C)
{
    bool Result = ((C == '\n') ||
                   (C == '\r'));

    return(Result);
}

static void
DeleteAllWhitespace(tokenizer *Tokenizer)
{
    for(;;)
    {
        if(IsWhitespace(Tokenizer->At[0]))
        {
            Tokenizer->At++;
        }
        else if((Tokenizer->At[0] == '/') &&
                (Tokenizer->At[1] == '/'))
        {
            Tokenizer->At += 2;
            while(Tokenizer->At[0] && !IsEndOfLine(Tokenizer->At[0]))
            {
                Tokenizer->At++;
            }

            if(IsEndOfLine(Tokenizer->At[0]))
            {
                Tokenizer->At++;
            }
        }
        else if((Tokenizer->At[0] == '/') &&
                (Tokenizer->At[1] == '*'))
        {
            Tokenizer->At += 2;
            while(Tokenizer->At[0] &&
                  !((Tokenizer->At[0] == '*') &&
                    (Tokenizer->At[1] == '/')))
            {
                Tokenizer->At++;
            }

            if(Tokenizer->At[0] == '*')
            {
                Tokenizer->At += 2;
            }
        }
        else
        {
            break;
        }
    }
}

static token
GetToken(tokenizer *Tokenizer)
{
    DeleteAllWhitespace(Tokenizer);

    token Token = {};
    Token.StringLength = 1;
    Token.String = Tokenizer->At;

    char Symbol = Tokenizer->At[0];
    Tokenizer->At++;
    switch(Symbol)
    {
        case '\0': { Token.Type = Token_EndOfFile; } break;

        case ',': { Token.Type = Token_Comma; } break;
        case '(': { Token.Type = Token_OpenParen; } break;
        case ')': { Token.Type = Token_CloseParen; } break;
        case ':': { Token.Type = Token_Colon; } break;
        case ';': { Token.Type = Token_Semicolon; } break;
        case '*': { Token.Type = Token_Asterisk; } break;
        case '[': { Token.Type = Token_OpenBracket; } break;
        case ']': { Token.Type = Token_CloseBracket; } break;
        case '{': { Token.Type = Token_OpenBrace; } break;
        case '}': { Token.Type = Token_CloseBrace; } break;

        case '"': 
        {   
            Token.Type = Token_String;
            Token.String = Tokenizer->At;
            while(Tokenizer->At[0] &&
                  (Tokenizer->At[0] != '"'))
            {
                if((Tokenizer->At[0] == '\\') &&
                   (Tokenizer->At[1]))
                {
                    Tokenizer->At++;
                }
                Tokenizer->At++;
            }

            Token.StringLength = Tokenizer->At - Token.String;
            if(Tokenizer->At[0] == '"')
            {
                Tokenizer->At++;
            }
        } break;

        default:
        {
            if(IsAlpha(Symbol) || (Tokenizer->At[0] == '_'))
            {
                Token.Type = Token_Identifier;

                while(IsAlpha(Tokenizer->At[0]) ||
                      IsNumber(Tokenizer->At[0]) || 
                      (Tokenizer->At[0] == '_'))
                {
                    Tokenizer->At++;
                }

                Token.StringLength = Tokenizer->At - Token.String;
            }
            else
            {
                Token.Type = Token_Unknown;
            }

        }
    }

    return(Token);
}

static bool
TokenStringEquals(token Token, char *String)
{
	for(int Index = 0;
        Index < Token.StringLength;
        Index++, String++)
    {
        if((*String == 0) ||
           (Token.String[Index] != *String))
        {
            return(false);
        }
    }

    bool Result = (*String == 0);
    return(Result);
}

std::vector<std::string> ShaderNames;

static void
ParseAndWriteShader(token ShaderName)
{
    char *ShaderDirectoryPath = "../data/shaders/"; 
    
    char Buffer[128];

    std::string NameString = std::string(ShaderName.String, ShaderName.StringLength);
    if(std::find(ShaderNames.begin(), ShaderNames.end(), NameString) == ShaderNames.end())
    {
        ShaderNames.push_back(NameString);

        _snprintf(Buffer, sizeof(Buffer), "%s%s.glsl", ShaderDirectoryPath, NameString.c_str());
        char *ShaderCode = ReadEntireFileAndNullTerminate(Buffer);

        printf("global_variable char %s[] = \n", NameString.c_str());

        char *LineStart = ShaderCode;
        while(LineStart)
        {
            printf("\"");

            char *At = LineStart;
            while(*At && !IsEndOfLine(*At)) { At++; }
            size_t LineLength = At - LineStart;
            printf("%.*s", (int)LineLength, LineStart);
            printf("\\n");

            printf("\"");

            printf("\n");

            if(IsEndOfLine(*At))
            {
                At += 2;
                LineStart = At;
            }
            else
            {
                break;
            }
        }

        printf(";\n");

        free(ShaderCode);
    }
}

static void
ParseCompileShader(tokenizer *Tokenizer)
{
    while(GetToken(Tokenizer).Type != Token_Comma);

    token NameVS = GetToken(Tokenizer);
    token Comma = GetToken(Tokenizer);
    token NameFS = GetToken(Tokenizer);

    // NOTE(georgy): Checks if this CompileShader token is CompileShader function definition
    if(!TokenStringEquals(NameVS, "char"))
    {
        ParseAndWriteShader(NameVS);
        ParseAndWriteShader(NameFS);
    }
}

int 
main(int ArgCount, char **Args)
{
    char *FileNames[] = 
    {
        "voxel_engine.hpp",
        "voxel_engine_debug.hpp",
        "voxel_engine_render.hpp"
    };

    for(int FileIndex = 0;
        FileIndex < (sizeof(FileNames)/sizeof(FileNames[0]));
        FileIndex++)
    {
        char *FileAsString = ReadEntireFileAndNullTerminate(FileNames[FileIndex]);

        tokenizer Tokenizer;
        Tokenizer.At = FileAsString;

        bool Parsing = true;
        while(Parsing)
        {
            token Token = GetToken(&Tokenizer);

            switch(Token.Type)
            {
                case Token_EndOfFile:
                {
                    Parsing = false;
                } break;
                
                case Token_Identifier:
                {
                    if(TokenStringEquals(Token, "CompileShader"))
                    {
                        ParseCompileShader(&Tokenizer);
                    }
                } break;
            }
        }

        free(FileAsString);
    }

    return(0);
}