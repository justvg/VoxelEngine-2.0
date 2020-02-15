#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#pragma pack(push, 1)
struct vea_header
{
#define VEA_MAGIC_VALUE (((u32)('v') << 0) | ((u32)('e') << 8) | ((u32)('a') << 16) | ((u32)('f') << 24))
//    u32 MagicValue;

//    u32 AssetCount;
   // u32 TagCount;

   // u64 
};
#pragma pack(pop)

int
main(int ArgCount, char **Args)
{
    FILE *File = fopen("data0.vea", "wb");
    if(File)
    {

    }
    else
    {
        printf("ERROR: Can't open the file!\n");
    }

    return(0);
}