#include <iostream>
//#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "lib_json_0.5/json.h"
#include <malloc.h>

#include "data_manager.h"

#include <sys/ioctl.h>
#include  <net/if.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <sys/stat.h>

#include <pthread.h> 

static unsigned mUserID =  0;
static std::string mNickName;
static std::string sErrorMsg;

 char *DataManager::user_id=NULL;
 char *DataManager::password =NULL;

static DataManager::MusicLib sLib;
CURL * DataManager::mCurl = NULL;
CURL * DataManager::pLogoCurl = NULL;

SongInfo  * DataManager::pDownList=NULL;

SongInfo DataManager::SongRndList[XIAMI_MAX_RND_NUM];
SongInfo DataManager::BillSongList[XIAMI_BILLS_NUM][XIAMI_BILL_SONG_NUM];
SongInfo DataManager::RadioSongList[XIAMI_RADIO_NUM][XIAMI_RADIO_SONG_NUM];
SongInfo DataManager::MySongs[XIAMI_MY_SONGS_PAGE];
AlbumInfo  DataManager::MyAlbums[XIAMI_MY_SONGS_PAGE];

unsigned int DataManager::BillIndex =0;
unsigned int DataManager::RadioIndex =0;
// for on line parse info
unsigned int DataManager::retSongNum =0;
unsigned int DataManager::retCollectNum =0;
unsigned int DataManager::retFavCollectNum=0;
unsigned int DataManager::retAlbumNum=0;
unsigned int DataManager::retArtistNum=0;
unsigned int DataManager::artist_total_album=0;

bool DataManager::start_dl_flag=false;
bool DataManager::restart_dl_flag=false;

static const unsigned int radio_id[2] = {144227, 138879};
static const char *prefix = "/tmp/cache/";

static void get_last_name(const char *pUrl, char *pName)
{
    const char *pSlash;
    const char *pDot;
    int j=0;    
    if(!pUrl || !pName)
        return;

    pSlash = strrchr(pUrl, '/');
    if(!pSlash)
        return;
            
    pDot = strrchr(pSlash, '.');
    if(!pDot)
        return;        
    
    while(pSlash != pDot) {
        pSlash++;
        if(pSlash == pDot)
            break;
            pName[j] = *pSlash;
            j++;
    }
    pName[j++] = '\0';
}

static int parse_songs(Json::Value &songs, SongInfo *pSongs, unsigned int max_size)
{
    unsigned int i;
    unsigned int size =0;
    
    if(!pSongs)
        return 0;
    
    pSongs->size = songs.size();
    for(i=0; i < songs.size() && i < max_size; i++) 
    {
        size = songs[i]["name"].asString().size();
        pSongs->name = (const char *) realloc((void *)pSongs->name, size +1);
        memset((void *)pSongs->name, 0, size+1);
        memcpy((void *)pSongs->name, (const void *)songs[i]["name"].asString().c_str(), size);
                    
        size = songs[i]["location"].asString().size();
        pSongs->location= (const char *) realloc((void *)pSongs->location, size +1);
        memset((void *)pSongs->location, 0, size+1);
        memcpy((void *)pSongs->location, (const void *)songs[i]["location"].asString().c_str(), size);

        size = songs[i]["album_logo"].asString().size();
        pSongs->album_logo= (const char *) realloc((void *)pSongs->album_logo, size +1);
        memset((void *)pSongs->album_logo, 0, size+1);
        memcpy((void *)pSongs->album_logo, (const void *)songs[i]["album_logo"].asString().c_str(), size);

        size = songs[i]["title"].asString().size();
        pSongs->album_title= (const char *) realloc((void *)pSongs->album_title, size +1);
        memset((void *)pSongs->album_title, 0, size+1);
        memcpy((void *)pSongs->album_title, (const void *)songs[i]["title"].asString().c_str(), size);
                    
        if(songs[i]["lyric"].isNull())
        {
            pSongs->lyric= (const char *) realloc((void *)pSongs->lyric, 5);
            memset((void *)pSongs->lyric, 0, 5);
            memcpy((void *)pSongs->lyric, "NULL", 4);
        }else {
            size = songs[i]["lyric"].asString().size();
            pSongs->lyric= (const char *) realloc((void *)pSongs->lyric, size +1);
            memset((void *)pSongs->lyric, 0, size+1);
            memcpy((void *)pSongs->lyric, (const void *)songs[i]["lyric"].asString().c_str(), size);
        }
	pSongs++;
    } 
    
    return 1;
}

static int parse_albums(Json::Value &albums, AlbumInfo *pAlbums)
{
    unsigned int i=0;
    unsigned int size =0;
    std::string id_str, cnt_str;    
        
    if(!pAlbums)
        return 0;

    for(i=0; i < albums.size() && i < XIAMI_MY_SONGS_PAGE; i++){    
        size = albums[i]["title"].asString().size();
        pAlbums->title = (char *) realloc((void *)pAlbums->title, size +1);
        memset((void *)pAlbums->title, 0, size+1);
        memcpy((void *)pAlbums->title, (const void *)albums[i]["title"].asString().c_str(), size);
        
        size = albums[i]["album_logo"].asString().size();
        pAlbums->logo = (char *) realloc((void *)pAlbums->logo, size +1);
        memset((void *)pAlbums->logo, 0, size+1);
        memcpy((void *)pAlbums->logo, (const void *)albums[i]["album_logo"].asString().c_str(), size);

        if(albums[i].isMember("artist_id")) {
            id_str = albums[i]["artist_id"].asString();
            pAlbums->id = atoi(id_str.c_str());            
         }
        else if(albums[i].isMember("album_id")) {
            id_str = albums[i]["album_id"].asString();
            pAlbums->id = atoi(id_str.c_str());                        
        }
        else {
            id_str = albums[i]["obj_id"].asString();
            pAlbums->id = atoi(id_str.c_str());
        }
        if(albums[i].isMember("songs_count")) {
            cnt_str = albums[i]["songs_count"].asString();
            pAlbums->songs_cnt = atoi(cnt_str.c_str());
        }
        pAlbums++;
    }
    return 1;
}

static void songs_init(SongInfo *pSongs, unsigned int size)
{
    unsigned int i;    
    for(i=0; i<size; i++) {
        pSongs->name = (char *) malloc(1);
        pSongs->location= (char *) malloc(1);
        pSongs->album_logo =(char *) malloc(1);
        pSongs->lyric=(char *) malloc(1);
        pSongs->singers=(char *) malloc(1);
        pSongs->album_title=(char *) malloc(1);
        pSongs++;
    }
}

static void songs_free(SongInfo *pSongs, unsigned int size)
{
    unsigned int i;    
    for(i=0; i<size; i++){
        free((void *)pSongs->name);
        free((void *)pSongs->location);
        free((void *)pSongs->album_logo);
        free((void *)pSongs->lyric);
        free((void *)pSongs->album_title);
        pSongs++;
    }
}

DataManager::DataManager():mLogged(false)
{
    unsigned int i;
    curl_global_init(CURL_GLOBAL_ALL);
    mCurl = curl_easy_init();
    pLogoCurl = curl_easy_init();
    start_dl_flag = false;

    user_id = (char *) malloc(1);
    password =(char *) malloc(1);

    songs_init(SongRndList, XIAMI_MAX_RND_NUM);
    for(i=0; i<XIAMI_BILLS_NUM; i++)
   {
        songs_init(BillSongList[i], XIAMI_BILL_SONG_NUM);
   }
    for(i=0; i<XIAMI_RADIO_NUM; i++)
   {
        songs_init(RadioSongList[i], XIAMI_RADIO_SONG_NUM);
    }
    songs_init(MySongs, XIAMI_MY_SONGS_PAGE);
    for(i=0; i<XIAMI_MY_SONGS_PAGE; i++)
    {        
        MyAlbums[i].logo = (char *) malloc(1);
        MyAlbums[i].title = (char *) malloc(1);
        MyAlbums[i].id = 0;
        MyAlbums[i].songs_cnt = 0;
    }    
    // check the download directory, 
    struct stat st;
    if(stat(prefix, &st) != 0)
    {
        int status;
        status = mkdir(prefix, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if(status != 0)
            printf("mkdir failed\n");
    }
    // create a thread to download the lyric and logo
    pthread_t  dl_thread;       
    pthread_create(&dl_thread,  NULL,  &(DataManager::do_dl_thread), NULL);
 }

DataManager::~DataManager()
{
    int i;
    
    curl_easy_cleanup(mCurl);
    curl_easy_cleanup(pLogoCurl);
    
    songs_free(SongRndList, XIAMI_MAX_RND_NUM);
    songs_free(MySongs, XIAMI_MY_SONGS_PAGE);
    free(user_id);
    free(password);
    for(i=0; i<XIAMI_MY_SONGS_PAGE; i++)
    {            
            free((void *)MyAlbums[i].title);
            free((void *)MyAlbums[i].logo);
    }
    for(i=0; i<XIAMI_BILLS_NUM; i++)
   {
        songs_free(BillSongList[i], XIAMI_BILLS_NUM);
    }
    
    for(i=0; i<XIAMI_RADIO_NUM; i++)
   {
        songs_free(RadioSongList[i], XIAMI_RADIO_SONG_NUM);
   }
}

size_t DataManager::write_file(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t written;
    
    written = fwrite(ptr, size, nmemb, stream);    
    return written;
}

 size_t DataManager::WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
	size_t realsize = size * nmemb;
	
  	struct MemoryStruct *mem = (struct MemoryStruct *)data;
    
  	mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory == NULL) {
    		/* out of memory! */ 
    		printf("not enough memory (realloc returned NULL)\n");   
		return 0;
	}
	  memcpy(&(mem->memory[mem->size]), ptr, realsize);
	  mem->size += realsize;
	  mem->memory[mem->size] = 0;
	 
	  return realsize;
}

//@return 0 --- sucessful, -1 ---  failed get ip address, -2 --- ping failed
int DataManager::ping_xiami(const char *ifname)
{
    struct ifreq s;
    int ret = -1;
    
    int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    strcpy(s.ifr_name, ifname);
    ret = ioctl(fd, SIOCGIFADDR, &s);
    if (0 == ret){
        printf("%s\n", inet_ntoa(((struct sockaddr_in *)&s.ifr_addr)->sin_addr));
       ret=system("ping -c 1 www.xiami.com"); // ping xiami website
       if( 0 == ret)
            return 0;
       else
            return -2;
    }
    return -1;
}

// a general function to get data from a URL
// @pUrl ---  to connect url
// @id    ---- your accout's ID from the login return, if not required, fill 0
bool DataManager::GetDataFromUrl(const char *pUrl, xiami_json_parse_callback parse)
{
    int ret = -1;
    struct MemoryStruct chunk;
    
    chunk.memory =(char *) malloc(1);  /* will be grown as needed by the realloc above */ 
    chunk.size = 0;    

    ret = curl_easy_setopt(mCurl, CURLOPT_URL, pUrl);
    if(0 != ret)
    {
        printf("set opt url error\n");
        return false;
    }
    /* send all data to this function  */ 
    curl_easy_setopt(mCurl, CURLOPT_WRITEFUNCTION, &(DataManager::WriteMemoryCallback)); 
	
    /* we pass our 'chunk' struct to the callback function */ 
    curl_easy_setopt(mCurl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(mCurl, CURLOPT_TIMEOUT, HTTP_CONNECTION_TIMEOUT);
    ret = curl_easy_perform(mCurl);
    if (ret  != CURLE_OK)
    {
        printf("perform error\n");
        return false;
    }
    bool status=false;
    
    //parse the json array
    status = parse(&chunk);
    free((void *)chunk.memory);
    
    return status;
}

bool DataManager::GetByHttpBasic(const char *pUrl, xiami_json_parse_callback parse)
{
    int ret = -1;
    struct MemoryStruct chunk;
    
    chunk.memory =(char *) malloc(1);  /* will be grown as needed by the realloc above */ 
    chunk.size = 0;    
    
    char user[128] ={0};
    memset(user, 0, 128);

    if(mUserID == 0) {
        printf("Please login before do collect\n");
        return false;
     }
    memcpy(user, user_id,  strlen(user_id));
    memcpy(user+strlen(user_id), ":", 1);
    memcpy(user+strlen(user_id)+1, password, strlen(password));
        
    printf("**** user=%s, user_id=%s, pwd=%s\n", user, user_id, password);
    ret = curl_easy_setopt(mCurl, CURLOPT_URL, pUrl);
    if(0 != ret)
    {
        printf("set opt url error\n");
        return false;
    }
    /* send all data to this function  */ 
    curl_easy_setopt(mCurl, CURLOPT_WRITEFUNCTION, &(DataManager::WriteMemoryCallback)); 
    /* we pass our 'chunk' struct to the callback function */ 
    curl_easy_setopt(mCurl, CURLOPT_WRITEDATA, (void *)&chunk);
    // add basic auth
    curl_easy_setopt(mCurl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
    curl_easy_setopt(mCurl, CURLOPT_USERPWD, user);
    curl_easy_setopt(mCurl, CURLOPT_TIMEOUT, HTTP_CONNECTION_TIMEOUT);
    ret = curl_easy_perform(mCurl);
    if (ret  != CURLE_OK)
    {
        printf("perform error\n");
        return false;
    }
    bool status =false;
    //parse the json array
    status=parse(&chunk);
    free((void *)chunk.memory);
        
    return status;
}

// example: successful return
//{
//  status: "ok"
//  user_id: "4460056"
//  nick_name: "shengdatest"
//  url: "122.225.11.135"
//  client_port: 4670
//}
/////////////// Parse functions ////////////////////////////
int  DataManager::ParseLoginInfo(struct MemoryStruct *pJson)
{
    Json::Reader reader;
    Json::Value value;

    const char* OK = "ok";
    const char* USER_ID  = "user_id";
    const char* NICK_NAME = "nick_name";

    std::string out;	

    printf("*** Longin-->>");
    if (reader.parse((const char*)pJson->memory, value)) {
        out = value["status"].asString();		
	if (out == OK) {
            mUserID  = atoi(value[USER_ID].asString().c_str());
            mNickName = value[NICK_NAME].asString();
            std::cout<<mNickName<<std::endl;
        }
        else {
		    sErrorMsg = value["msg"].asString();
                   std::cout<<sErrorMsg<<std::endl;
                return false;
        }
    } 
   	return  true;
}

int DataManager::ParseLibInfo(struct MemoryStruct *pJson)
{
	Json::Reader reader;
	Json::Value value;
    
	 if (reader.parse((const char*)pJson->memory, value))
	{
		const char* OK = "ok";
		const char* FAIL = "failed";
		std::string out = value["status"].asString();
		if (out == OK)
		{
			Json::Value info = value["count"];
			sLib.albumNum = atoi(info["albums"].asString().c_str());
			sLib.artistNum =  atoi(info["artists"].asString().c_str());
			sLib.collectNum = atoi(info["collects"].asString().c_str());
			sLib.radioNum = atoi(info["radios"].asString().c_str());
			sLib.songNum = atoi(info["songs"].asString().c_str());
                    sLib.favCollectNum= atoi(info["fav_collects"].asString().c_str());
		}
		else
		{
			if(out == FAIL)
			{
				sErrorMsg = value["msg"].asString();
                        return false;
			}
		}
		std::cout<<out<<std::endl;
	  }
   	return true;
}

/*
 example for favoorite info
 *** ParseFavoriteInfo {"status":"ok","songs":[
{"song_id":"1770088581", "album_id":"430596","name":"Kiss Kiss Kiss","style":null,"track":"1","artist_name":"\u90d1\u878d","artist_id":"2521",
"lyric":"http:\/\/img.xiami.com\/.\/lyric\/upload\/81\/1770088581_1306767164.lrc",
"inkui_url":"\/2521\/430596\/1770088581.mp3",
"songwriters":null,"composer":null,"threads_count":"0","posts_count":"0",
"listen_file":"\/2521\/430596\/01 1770088581_2098260.mp3","lyric_url":"0",
"has_resource":null,"cd_serial":"1","sub_title":null,"complete":null,"singers":"\u90d1\u878d",
"arrangement":null,"default_resource_id":"2098260","lyric_file":null,"title_url":"Kiss+Kiss+Kiss",
"length":"212","recommends":"221","grade":"3",
"title":"Kiss Kiss Kiss",
"album_logo":"http:\/\/img.xiami.com\/.\/images\/album\/img21\/2521\/4305961300172212_2.jpg",
"location":"http:\/\/f1.xiami.net\/2521\/430596\/01 1770088581_2098260.mp3","low_size":"2545371",
"file_size":"6147852","low_hash":"9ecbd6391d246d68999549b6b4a60b0d","whole_hash":"00c291ff1735682ac3caa5b688f16382",
"content_hash":"00c291ff1735682ac3caa5b688f16382","content_size":"6147852","category":"\u534e\u8bed","lock_lrc":"2","year_play":"45712"},
*/
int DataManager::ParseFavoriteInfo(struct MemoryStruct *pJson)
{
	//printf(" *** ParseFavoriteInfo %s\n", pJson->memory);
	Json::Reader reader;
	Json::Value root;
	bool ret;

	ret = reader.parse((const char*)pJson->memory, root);
	 if (!ret)
	{
	    // report to the user the failure and their locations in the document.
	    std::cout  << "Failed to parse configuration\n"
               << reader.getFormatedErrorMessages();
		return  0;
	 }
	bool isNull = root["status"].isNull();
	if (isNull)
	{
		printf("get songs null\n");
		return 0;
	}
    Json::Value songs;
    ret = false;
    ret=root.isMember("album");
    if(ret){
        Json::Value album = root["album"];
        songs = album["songs"];        
    }else {
        songs = root["songs"];
    }
    retSongNum = songs.size();
    parse_songs(songs, MySongs, XIAMI_MY_SONGS_PAGE);
    
    return 1;	
}

int DataManager::ParseAlbumsInfo(struct MemoryStruct *pJson)
{
	Json::Reader reader;
	Json::Value root;
	bool ret;

	ret = reader.parse((const char*)pJson->memory, root);
	 if (!ret)
	{
	    std::cout  << "Failed to parse configuration\n"
               << reader.getFormatedErrorMessages();
		return  0;
	 }
	bool isNull = root["status"].isNull();
	if (isNull)
	{
		printf("get ParseAlbumsInfo null\n");
		return 0;
	}
    bool flags=false;
    int size =0;
    int i=0;    
    
    flags = root.isMember("collects");
    if(flags) {
        Json::Value collects;
        std::string isMyCollect;
        const char *mine = "true";
        retCollectNum=0;    
        retFavCollectNum=0;
        
        printf("*** parse collects\n");
        collects = root["collects"];
        if(collects.isNull())
            return 0;
        size = collects.size();
        isMyCollect = collects[i]["ismycollect"].asString();
        if(mine == isMyCollect) {
            retFavCollectNum = size;
            printf("*** favCollectNum size=%d\n", retFavCollectNum);
         }
        else  {            
            retCollectNum= size;
            printf("*** collectNum size=%d\n", retCollectNum);
         }
        parse_albums(collects, MyAlbums);
    }
    flags = root.isMember("albums");
    if(flags) {
        printf("*** parse albums\n");                
        Json::Value albums = root["albums"];
        retAlbumNum =0;
        artist_total_album=0;
        if(albums.isNull())
            return 0;
        if(root.isMember("albums_count")){
            std::string albums_cnt;
            albums_cnt = root["albums_count"].asString();
            artist_total_album = atoi(albums_cnt.c_str());
        }
        retAlbumNum= albums.size();
        parse_albums(albums, MyAlbums);     
    }
    flags = root.isMember("artists");
    if(flags) {
        printf("*** parse artists\n");
        Json::Value artists = root["artists"];
        std::string alb_cnt;
        retArtistNum =0;
        if(artists.isNull())
            return 0;
        alb_cnt = root["albums_count"].asInt();
        retAlbumNum = atoi(alb_cnt.c_str()); // artist's total album number
        retArtistNum = artists.size();
        parse_albums(artists, MyAlbums);
    }
    return 1;	
}

int  DataManager::ParseRndInfo( struct MemoryStruct *pJson)
{
	Json::Reader reader;
	Json::Value value;
	bool ret;
	
	ret = reader.parse((const char*)pJson->memory, value);
	 if (!ret)
	{
	       // report to the user the failure and their locations in the document.
		std::cout  << "Failed to parse configuration\n"<< reader.getFormatedErrorMessages();
		return  false;
	 }
	if (value["status"].isNull())
	{
		printf("get songs null\n");
		return 0;
	}
	Json::Value songs = value["songs"];
	SongInfo *pSongs = SongRndList;
       pSongs->size = songs.size();
       return parse_songs(songs, pSongs, XIAMI_MAX_RND_NUM);
}

int DataManager::ParseBills(struct MemoryStruct *pJson)
{
	unsigned int i;
	Json::Reader reader;
	Json::Value root;
	bool ret;
	std::string name;
        
	ret = reader.parse((const char*)pJson->memory, root);
	 if (!ret)
	{
	       // report to the user the failure and their locations in the document.
		std::cout  << "Failed to parse configuration\n"<< reader.getFormatedErrorMessages();
		return  false;
	 }
	bool isNull = root["status"].isNull();
	if (isNull)
	{
		printf("get bills null\n");
		return false;
	}
       name = root["name"].asString();
       std::cout<<"name:"<<name<<std::endl;        
       
	Json::Value bills = root["data"];
       Json::Value sub_bills;
    
	std::string location;
	std::string category_name;
       unsigned int  j;
       
       printf("bill. size=%d\n", bills.size());
	for(i=0; i < bills.size(); i++) 
	{
       	name = bills[i]["name"].asString();
              std::cout<<"name:"<<name<<std::endl;
              sub_bills = bills[i]["bills"];
              printf("sub bills size=%d\n", sub_bills.size());
              for(j=0; j<sub_bills.size(); j++)
              {
                name=sub_bills[j]["name"].asString();
                std::cout<<"sub name:"<<name<<std::endl;
              }
        }
	return true;
}

int DataManager::ParseSubBills(struct MemoryStruct *pJson)
{
	Json::Reader reader;
	Json::Value root;
	bool ret;
	std::string name;

	//printf(" *** Line :%d ParseSubBills %s\n", __LINE__, pJson->memory);
	ret = reader.parse((const char*)pJson->memory, root);
	 if (!ret)
	{
		std::cout  << "Failed to parse configuration\n"<< reader.getFormatedErrorMessages();
		return  false;
	 }
	bool isNull = root["status"].isNull();
	if (isNull)
	{
		printf("get bills null\n");
		return false;
	}        
        Json::Value songs;
        std::string location;
        songs = root["songs"];
        SongInfo *pSongs = BillSongList[0];
        return parse_songs(songs, pSongs, XIAMI_BILL_SONG_NUM);
}

int DataManager::ParseRadio(struct MemoryStruct *pJson)
{
	Json::Reader reader;
	Json::Value value;
	bool ret;
    
	//printf(" *** Line :%d ParseSubBills %s\n", __LINE__, pJson->memory);
	ret = reader.parse((const char*)pJson->memory, value);
	 if (!ret)
	{
	       // report to the user the failure and their locations in the document.
		std::cout  << "Failed to parse configuration\n"<< reader.getFormatedErrorMessages();
		return  false;
	 }
	if (value["status"].isNull())
	{
		printf("get songs null\n");
		return 0;
	}
       Json::Value root = value["radio"];
	Json::Value songs = root["songs"];
	SongInfo *pSongs = RadioSongList[RadioIndex];
       return parse_songs(songs, pSongs, XIAMI_RADIO_SONG_NUM);
}

bool DataManager::GetMusicLibInfo(MusicLib& lib)
{
//    const char* URLFormat = "http://api.xiami.com/app/android/lib?uid=%d";
    const char URLFormat[] ="http://api.xiami.com/app/android/lib?uid=%d&from=snda";
       char url[256] ={0};
       bool ret = false;
       
	sprintf(url, URLFormat, mUserID);
       ret = GetDataFromUrl(url, &(DataManager::ParseLibInfo));
       if(ret)
            printf("uid=%d songs=%d albums=%d collects=%d fav=%d\n", mUserID, sLib.songNum,  sLib.albumNum, sLib.collectNum, sLib.favCollectNum);
       
       lib = sLib;
	return ret;
}

bool DataManager::downloadSong( const char *pFileName,  const char *pUrl, int file_type)
{	
    FILE *fp;
    int ret = 0;
    int size;
    
    char postfix[4] = {0};
    
    if(!pFileName || !pUrl )
    {
        printf("Null pFilename or pUrl\n");
        return false;
    }
    
    switch(file_type)
    {
        case FILE_TYPE_MP3:
            strcpy(postfix, ".mp3");
            break;
         case FILE_TYPE_LOGO:
            strcpy(postfix, ".jpg");
            break;
         case FILE_TYPE_LYRIC:
            strcpy(postfix, ".lrc");
            break;
         default:
             strcpy(postfix, ".mp3");
            break;
    }
    size = strlen(prefix) + strlen(pFileName) +5;
    char path_name[256] = {0}; 

    memset(path_name, 0, size);
    memcpy(path_name, prefix, strlen(prefix));
    memcpy(path_name+strlen(prefix), pFileName, strlen(pFileName));    
    memcpy(path_name+strlen(prefix)+strlen(pFileName), postfix, sizeof(postfix));
    path_name[size]='\0';    
    printf("download :: pFileNmae=%s, name=%s, size=%d\n", pFileName, path_name, size);
    if(access(path_name, F_OK) == 0)
    {
            printf(" file exist return \n");
            return true;
    }
    fp = fopen(path_name, "wb");
    if(NULL == fp)
    {
        printf("open file error\n");
        return false;
    }
    if (mCurl) {
        curl_easy_setopt(pLogoCurl, CURLOPT_URL, pUrl);
        curl_easy_setopt(pLogoCurl, CURLOPT_WRITEFUNCTION, &(DataManager::write_file));
        curl_easy_setopt(pLogoCurl, CURLOPT_WRITEDATA, fp);        
        //curl_easy_setopt(mCurl, CURLOPT_NOPROGRESS, FALSE);        
	// Install the callback function
	//curl_easy_setopt(mCurl, CURLOPT_PROGRESSFUNCTION, progress_func);
        ret = curl_easy_perform(pLogoCurl);
        if(ret != CURLE_OK)
        {
            fclose(fp);
            return false;
        }
    }
     fclose(fp);
    return true;
}

bool DataManager::downUrl( const char *pFileName,  const char *pUrl)
{	
    FILE *fp;
    int ret = 0;
    
    if(!pFileName || !pUrl )
    {
        printf("Null pFilename or pUrl\n");
        return false;
    }
    printf("downUrl :: pFileNmae=%s\n", pFileName);
    if(access(pFileName, F_OK) == 0)
    {
            printf(" file exist return \n");
            return true;
    }
    fp = fopen(pFileName, "wb");
    if(NULL == fp)
    {
        printf("open file error\n");
        return false;
    }
    if (mCurl) {
        curl_easy_setopt(pLogoCurl, CURLOPT_URL, pUrl);
        curl_easy_setopt(pLogoCurl, CURLOPT_WRITEFUNCTION, &(DataManager::write_file));
        curl_easy_setopt(pLogoCurl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(mCurl, CURLOPT_TIMEOUT, HTTP_CONNECTION_TIMEOUT);    
        ret = curl_easy_perform(pLogoCurl);
        if(ret != CURLE_OK)
        {
            fclose(fp);
            return false;
        }
    }
     fclose(fp);
    return true;
}

 void *DataManager::do_dl_thread( void * parameter)
{
    int i;
    char buf[128];

    pthread_detach(pthread_self());
    
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex,NULL);
    
    SongInfo *pSongs = SongRndList;
    int size = 0;
    while(1) {
        if(start_dl_flag && pDownList) {
            pSongs = pDownList;
            size = pSongs->size;
                for(i=0; i<size; i++) {
                        if(restart_dl_flag)
                            break;
                        get_last_name(pSongs->location, buf);
                        if(pSongs->album_logo)
                            downloadSong(buf, pSongs->album_logo, FILE_TYPE_LOGO);
                        if(strlen(pSongs->lyric) > 6)
                             downloadSong(buf, pSongs->lyric, FILE_TYPE_LYRIC);
                        pSongs++;
                  }
                pthread_mutex_lock(&mutex);
                if(i == size) {
                    // turn off dl flag, finished
                    start_dl_flag = false;
                }else {
                    printf("*** restart dl list\n");
                    restart_dl_flag = false; // already update the dl list, restart 
                }
                pthread_mutex_unlock(&mutex);             
        }
        sleep(1);
    }
    return NULL;
}

bool DataManager::GetFavoriteInfo(const int pageNum, int type)
{
    //const char mySongs[] = "http://api.xiami.com/app/android/lib-songs?uid=%d&page=%d";
    // max cnt = 50, it support &cnt=? 
    const char mySongs[] = "http://api.xiami.com/app/android/lib-songs?uid=%d&page=%d&from=snda&cnt=5";
    const char myCollects[]="http://api.xiami.com/app/android/lib-collects?uid=%d&type=mine&page=%d&limit=5";
    const char myFavCollects[]="http://api.xiami.com/app/android/lib-collects?uid=%d&type=fav&page=%d&limit=5";
    const char myAlbums[]="http://api.xiami.com/app/android/lib-albums?uid=%d&from=snda&page=%d&cnt=5";
    const char myArtists[]="http://api.xiami.com/app/android/lib-artists?uid=%d&page=%d&from=snda&cnt=5";
    const char myRadios[]="http://api.xiami.com/app/android/lib-radios?uid=%d&page=%d";
    
    char url[256];
    memset(url, 0, 256);
    switch(type)
    {
        case MY_SOGNS:
            sprintf(url, mySongs, mUserID, pageNum);
            GetDataFromUrl(url, &DataManager::ParseFavoriteInfo);
            break;
        case MY_FAV_COLLECTS:
            sprintf(url, myFavCollects, mUserID,pageNum);
            GetDataFromUrl(url, &DataManager::ParseAlbumsInfo);
            break;
        case MY_COLLECTS:
            sprintf(url, myCollects, mUserID,pageNum);
            GetDataFromUrl(url, &DataManager::ParseAlbumsInfo);
            break;
        case MY_ALBUMS:
            sprintf(url, myAlbums, mUserID,pageNum);
            printf("*** get my albums\n");
            GetDataFromUrl(url, &DataManager::ParseAlbumsInfo);            
            break;
         case MY_ARTISTS:
            sprintf(url, myArtists, mUserID,pageNum);
            GetDataFromUrl(url, &DataManager::ParseAlbumsInfo);            
            break;
        case MY_RADIOS:
            sprintf(url, myRadios, mUserID,pageNum);
            printf("*** get my radios\n");
            //GetDataFromUrl(url, &DataManager::ParseFavoriteInfo);            
            break;
        default:
            return false;
    }
    
    return true;
}

//////////////////  action API functions
bool DataManager::Login(const char *account, const char *passwd)
{
    const char URLFormat[] = "http://api.xiami.com/app/android/login?email=%s&pwd=%s";
        
    if (NULL == account || NULL == passwd)
    {
        return false;
    }
    
    int size = 0;
    size = strlen(URLFormat) -4 + strlen(account) + strlen(passwd) + 1;	
    char* url = new char[size];
    
    sprintf(url, URLFormat, account, passwd);
    GetDataFromUrl(url, &DataManager::ParseLoginInfo);
    delete[] url; 

    user_id = (char *) realloc(user_id, strlen(account)+1);
    password = (char *)realloc(password, strlen(passwd)+1);

    memset(user_id, 0, strlen(account));
    memset(password, 0, strlen(password));
    
    memcpy(user_id, account, strlen(account));
    memcpy(password, passwd, strlen(passwd));

    printf("*** Login:: user_id=%s, password=%s\n", user_id, password);
    return mUserID != 0;
}

SongInfo * DataManager:: get_rnd_list(int &result)
{
     int ret = 0;
     bool rnd;
    const char *xiamiUrl = "http://api.xiami.com/app/android/sdornd";

    // move to QT 
//    ret=DataManager::ping_xiami("eth0");
    result = ret;
    
    if( ret < 0)
    {
        return (SongInfo *)NULL;
     }
    start_dl_flag = false;
     rnd = GetDataFromUrl(xiamiUrl, &DataManager::ParseRndInfo);
    if(rnd)
    {
	if(SongRndList[0].name) {
            pDownList = SongRndList;
            start_dl_flag = true;
            restart_dl_flag = true;
             return SongRndList;
       }
    }    
    return (SongInfo *)NULL;
}

SongInfo  * DataManager:: get_bills_list(int category, int sub_bill, int &result)
{
    // bill urls
    //const char bills_url[]="http://api.xiami.com/app/android/bills?from=shengda";
    //const char bills_detail[] ="http://api.xiami.com/app/android/bill?id=%d&from=shengda";

    //hot bill
    const char *hot_bill ="http://dev.xiami.com/app/android/music-hotplay?c=20";
    bool parse = false;

    parse = GetDataFromUrl(hot_bill, &DataManager::ParseSubBills);
    if(parse) {
        if(BillSongList[BillIndex][0].name) {
            result = XIAMI_BILL_SONG_NUM;
            pDownList = BillSongList[BillIndex];
            start_dl_flag = true;
            restart_dl_flag = true;
            return BillSongList[BillIndex];
        }
    }
    printf(" get_bills_list not found return null\n");
    return (SongInfo  *)NULL;
}

SongInfo  * DataManager:: get_radio_list(int channel, int index, int &result)
{
    //const char fm_category[]= "http://api.xiami.com/app/android/radio-category";
    const char fm_radio[] = "http://api.xiami.com/app/android/radio?id=%d";
    // xiami guest radio
//    const char guess_radio[] = "http://api.xiami.com/app/android/guess?uid=%d&from=snda";
    
    int size;
    int id = 0;
    char url[128] ={0};
    
    if( index > 2)
        return (SongInfo  *)NULL;

    id = radio_id[index];
    memset(url, 0, 128);
    bool parse;
    
    sprintf(url, fm_radio, id);
    RadioIndex = (unsigned  int)index;
    parse = GetDataFromUrl(url, &DataManager::ParseRadio); 
    if(parse){
        result = XIAMI_RADIO_SONG_NUM;    
        pDownList = RadioSongList[index];
        start_dl_flag = true;
        restart_dl_flag = true;        
        return RadioSongList[index];
    }    
    return (SongInfo *)NULL;
}

SongInfo  *DataManager::get_my_songs(int page, unsigned int &cnt)
{
    bool ret = false;
    
    ret = GetFavoriteInfo(page, MY_SOGNS);
    if(ret) {
        cnt = retSongNum;
        return MySongs;
    }
    return (SongInfo  *)NULL;
}

CollectInfo* DataManager::get_my_collects(int page, unsigned int &cnt)
{
    bool ret = false;
    
    ret = GetFavoriteInfo(page, MY_COLLECTS);
    if(ret){
        cnt = retFavCollectNum;
        return MyAlbums;
    }
    return (AlbumInfo *)NULL;
}

CollectInfo* DataManager::get_my_fav_collects(int page, unsigned int &cnt)
{
    bool ret = false;
    
    ret = GetFavoriteInfo(page, MY_FAV_COLLECTS);
    if(ret){
        cnt = retFavCollectNum;
        return MyAlbums;
    }
    return (CollectInfo *)NULL;
}

AlbumInfo *DataManager::get_my_albums(int page, unsigned int &cnt)
{
    bool ret = false;
    
    ret = GetFavoriteInfo(page, MY_ALBUMS);
    if(ret){
        cnt = retAlbumNum;
        return MyAlbums;
    }
    return (AlbumInfo *)NULL;
}

ArtistInfo * DataManager::get_my_artists(int page, unsigned int &cnt)
{
    bool ret = false;
    
    ret = GetFavoriteInfo(page, MY_ARTISTS);
    if(ret){
        cnt = retArtistNum;
        return MyAlbums;
    }
    return (ArtistInfo *)NULL;
}

SongInfo *DataManager::get_albums_songs(unsigned int albums_id, unsigned int page, unsigned int &cnt)
{
//    const char albumsSongs[] = "http://api.xiami.com/app/android/album?id=%d";
     const char albumsSongs[] ="http://api.xiami.com/app/android/album?id=%d&from=snda&page=%d";
    char url[256] ={0};
    bool ret = false;

    memset(url, 0, 256);
    sprintf(url, albumsSongs, albums_id, page);

    ret = GetDataFromUrl(url, &DataManager::ParseFavoriteInfo);
    if(ret) {
        cnt = retSongNum;
        return MySongs;
    }
    return (SongInfo  *)NULL;    
}

AlbumInfo *DataManager::get_artist_albums(unsigned int artist_id,  unsigned int page, unsigned int &cnt, unsigned int &total_albums)
{
//    const char artistAlbums[] = "http://api.xiami.com/app/android/artist?id=%d&page=%d";
//    const char albums_list_old[] = "http://api.xiami.com/app/android/artist-albums?id=%d&page=%d";
    const char albums_list[] = "http://api.xiami.com/app/android/artist-albums?id=%d&page=%d&from=snda";
    char url[256] ={0};
    bool ret = false;

    memset(url, 0, 256);
    sprintf(url, albums_list, artist_id, page);
    ret = GetDataFromUrl(url, &DataManager::ParseAlbumsInfo);
    if(ret){
        cnt = retAlbumNum;
         total_albums =artist_total_album;
        return MyAlbums;
    }
    return (AlbumInfo *)NULL;    
}

SongInfo *DataManager::search_songs(const char *pKeyWord, unsigned int page, unsigned int &cnt)
{
    const char search_url[] ="http://api.xiami.com/app/android/search?key=%s&page=%d";
    char url[256] ={0};
    bool ret = false;

    memset(url, 0, 256);
    if(!pKeyWord || cnt <0)
        return NULL;
    
    sprintf(url, search_url, pKeyWord, page);

    ret = GetDataFromUrl(url, &DataManager::ParseFavoriteInfo);
    if(ret) {
        cnt = retSongNum;
        return MySongs;
    }
    return (SongInfo  *)NULL;    

}

int DataManager::ParseStatusInfo(struct MemoryStruct *pJson)
{
    Json::Reader reader;
    Json::Value value;
    bool ret;
    std::string status;
    const char *ok="ok";
    const char *failed="failed";
    const char *fal="false";
    
    printf(" *** Line :%d ParseStatusInfo %s\n", __LINE__, pJson->memory);
    ret=reader.parse((const char*)pJson->memory, value);
    if(ret){
        status = value["status"].asString();		
	if(status == failed){
            std::string msg;
            msg=value["msg"].asString();
            std::cout<<msg<<std::endl;
        return false;
       }
        if(status == fal) {
            printf("status: false\n");
            return false;    
        }
       if(status == ok)
        return true;
    } 
   	return  false;
}

bool DataManager::collect_to_my_xiami(unsigned int id, int type, bool collect)
{
    //support type: album artist collect radio
    const char collect_url[] = "http://api.xiami.com/app/android/fav?id=%d&type=%s";
    // un-collect 
    const char unCollect_url[] ="http://api.xiami.com/app/android/unfav?id=%d&type=%s";
    // grade 0 : just do collect, don't give the grade
    // grade 1/2/3/4/5 
    // grade -1 : cancel the collect
    const char collect_song[] = "http://www.xiami.com/app/android/grade?id=%d&grade=0";
    const char unCollect_song[]="http://www.xiami.com/app/android/grade?id=%d&grade=-1";
    char url[256]={0};
    char type_name[10]={0};

    printf("collect\n");
    memset(url, 0, 256);
    switch(type){
        case COLLECT_TYPE_SONG:
            if(collect)
                sprintf(url, collect_song, id);
            else
                sprintf(url, unCollect_song, id);
            break;
        case COLLECT_TYPE_ALBUM:
            strcpy(type_name, "album");
            if(collect)
                sprintf(url, collect_url,id, type_name);
            else
                sprintf(url, unCollect_url,id, type_name);
            break;
        case COLLECT_TYPE_ARTIST:
            strcpy(type_name, "artist");
            if(collect)
                sprintf(url, collect_url,id, type_name);
            else
                sprintf(url, unCollect_url,id, type_name);
            break;
        case COLLECT_TYPE_RADIO:
            strcpy(type_name, "radio");
            if(collect)
                sprintf(url, collect_url,id, type_name);
            else
                sprintf(url, unCollect_url,id, type_name);
            break;
        default:
            return false;
    }
    printf("collect url=%s\n", url);
    return GetByHttpBasic(url, &DataManager::ParseStatusInfo);
}

bool DataManager::isMyFavSong(unsigned int song_id)
{
    const char isMySongUrl[] ="http://api.xiami.com/app/android/isfavsong?uid=%d&id=%d";
    char url[256] ={0};
    bool ret = false;

    memset(url, 0, 256);
    if(mUserID == 0){
        printf("Not login\n");
        return false;
    }
    sprintf(url, isMySongUrl, mUserID, song_id);
    return  GetDataFromUrl(url, &DataManager::ParseStatusInfo);
}

