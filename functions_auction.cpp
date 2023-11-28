#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <cstdlib>


/*int GetBidList(int AID, BIDLIST *list){
    struct dirent **filelist;
     int n_entries, n_bids, len;
     char dirname[20];
     char pathname[32];

    sprintf(dirname,"AUCTIONS/%03d/BIDS/", AID);
    n_entries = scandir(dirname, &filelist, 0, alphasort);
    if (n_entries <=0)return(0);
    n_bids =0;
    list->no_bids=0;
    while(n_entries--){
        len=strlen(filelist[n_entries]->d_name);
        if (len==10){
            sprintf(pathname,"AUCTIONS/%03d/BIDS/%s",AID,filelist[n_entries]->d_name);
            if(LoadBid(pathname,list))++n_bids;
        }
        free(filelist[n_entries]);
        if (n_bids==50)break;
    }
    free(filelist);
    return(n_bids);
}*/

//*------------------------------------------------------*
//*Given Functions*
//*------------------------------------------------------*
int CheckAssetFile(char *fname) {
    struct stat filestat;
    int retstat;

    retstat = stat(fname, &filestat);

    if (retstat == -1 || filestat.st_size == 0)
        return 0;

    return filestat.st_size;
}

int CreateAUCTIONDir(int AID) {
    char AID_dirname[15];
    char BIDS_dirname[20];
    int ret;

    if (AID < 1 || AID > 999)
        return 0;

    sprintf(AID_dirname, "AUCTIONS/%03d", AID);
    ret = mkdir(AID_dirname, 0700);

    if (ret == -1)
        return 0;

    sprintf(BIDS_dirname, "AUCTIONS/%03d/BIDS", AID);
    ret = mkdir(BIDS_dirname, 0700);

    if (ret == -1) {
        rmdir(AID_dirname);
        return 0;
    }

    return 1;
}

int CreateLogin(char *UID) {
    char loginName[35];
    FILE *fp;

    if (strlen(UID) != 6)
        return 0;
    
    sprintf(loginName, "USERS/%s/%s_login.txt", UID, UID);

    fp = fopen(loginName, "w");

    if (fp == NULL)
        return 0;

    fprintf(fp, "Logged in\n");

    fclose(fp);
    return 1;
}
int EraseLogin(char *UID) {
    char loginName[35];

    if (strlen(UID) != 6)
        return 0;

    sprintf(loginName, "USERS/%s/%s_login.txt", UID, UID);
    //erase pass??
    unlink(loginName);

    return 1;
}


//*------------------------------------------------------*
//*Created Functions*
//*------------------------------------------------------*

int Register(char *UID, char *pass){
    char userDir[18];
    char userPass[33];
    int ret;
    FILE *fp;

    //creating user directory
    sprintf(userDir, "ASDIR/USERS/%s", UID);
    ret = mkdir(userDir, 0700);
    if (ret == -1) {
        //need something else?
        return 0;
    }
    //saving password
    sprintf(userPass, "ASDIR/USERS/%s/%s_pass.txt",UID,UID);
    fp = fopen(userPass, "w");
    if (fp == NULL)
        return 0;

    fprintf(fp, "%s\n",pass);
    fclose(fp);
    return 1;
}
int main(int argc,char *argv[]){
    printf("%s %s",argv[1],argv[2]);
    Register(argv[1],argv[2]);
}
