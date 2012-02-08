#if !defined(DATAMANAGER_H)
#define DATAMANAGER_H

#define MAX_NAME_LENGHT 256
#include <string.h>
//#include "curl/curl.h"
#include <iostream>

#include "./curl-7.21.7/include/curl/curl.h"

struct MemoryStruct {
  char *memory;
  size_t size;
};

#define XIAMI_MAX_RND_NUM 20

#define XIAMI_BILLS_NUM 1
#define XIAMI_BILL_SONG_NUM 20

#define XIAMI_RADIO_NUM 2
#define XIAMI_RADIO_SONG_NUM 10

#define XIAMI_MY_SONGS_PAGE 25

#define HTTP_CONNECTION_TIMEOUT 5 // secends
enum{
    FILE_TYPE_MP3 = 0,
    FILE_TYPE_LOGO,
    FILE_TYPE_LYRIC
};

enum{
    MY_SOGNS =0,
    MY_FAV_COLLECTS,        
    MY_COLLECTS,
    MY_ALBUMS,
    MY_ARTISTS,
    MY_RADIOS
};

enum{
    COLLECT_TYPE_SONG=0,
    COLLECT_TYPE_ALBUM,
    COLLECT_TYPE_ARTIST,
    COLLECT_TYPE_RADIO
};

typedef struct Song{
    unsigned int  song_id;
    unsigned int size;
    const char *name;
    const char *album_title;
    const char *lyric;
    const char *singers;
    const char *album_logo;
    const char *location;
}SongInfo;

typedef struct Album{
    const char *title;
    const char *logo;
    unsigned int songs_cnt;
    unsigned int id;
}AlbumInfo;

typedef  struct Album CollectInfo;
typedef  struct Album ArtistInfo;

/* private callback pointer */
 typedef int (*xiami_json_parse_callback)(struct MemoryStruct *pJson); 

class DataManager
{
    private:
        static CURL *mCurl;
        static CURL *pLogoCurl;
        static char *user_id;
        static char *password;
        bool mLogged;
	 static bool start_dl_flag;
        static bool restart_dl_flag;
        static SongInfo *pDownList;
        static size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data);
        // music info parse function
        static int	ParseLoginInfo(struct MemoryStruct *pJson);
        static int	ParseLibInfo(struct MemoryStruct *pJson);
        static int	ParseFavoriteInfo(struct MemoryStruct *pJson);			
        static int	ParseRndInfo( struct MemoryStruct *pJson);
        static int	ParseSubBills(struct MemoryStruct *pJson);
        static int  ParseBills(struct MemoryStruct *pJson);
        static int  ParseAlbumsInfo(struct MemoryStruct *pJson);
        static int  ParseStatusInfo(struct MemoryStruct *pJson);

        static size_t  write_file(void *ptr, size_t size, size_t nmemb, FILE *stream);
        static int ping_xiami(const char *ifname);
        static  bool GetDataFromUrl(const char *pUrl, xiami_json_parse_callback parse);
        static bool  GetByHttpBasic(const char *pUrl, xiami_json_parse_callback parse);
        static int ParseRadio(struct MemoryStruct *pJson);
        static void * do_dl_thread( void * parameter);
        static bool GetFavoriteInfo(const int pageNum, int type);
        static bool  downloadSong(const char *pFileName,  const char *pUrl, int  file_type);

        static SongInfo SongRndList[XIAMI_MAX_RND_NUM];
        static SongInfo BillSongList[XIAMI_BILLS_NUM][XIAMI_BILL_SONG_NUM];
        static SongInfo RadioSongList[XIAMI_RADIO_NUM][XIAMI_RADIO_SONG_NUM];
        static SongInfo MySongs[XIAMI_MY_SONGS_PAGE];
        static  AlbumInfo MyAlbums[XIAMI_MY_SONGS_PAGE];
        
        static unsigned int BillIndex;
        static unsigned int RadioIndex;

        static unsigned int retSongNum;
        static unsigned int retCollectNum;
        static unsigned int retFavCollectNum;
        static unsigned int retAlbumNum;
        static unsigned int retArtistNum;
        static unsigned int  artist_total_album;
    public:
        DataManager();
        ~DataManager();
        class MusicLib
        {
            public:
                int songNum;
                int collectNum;
                unsigned int  favCollectNum;
                int albumNum;
                int artistNum;
                int radioNum;
                MusicLib operator = (const MusicLib lib)
                {
                    songNum = lib.songNum;
                    collectNum = lib.collectNum;
                    albumNum = lib.albumNum;
                    artistNum = lib.artistNum;
                    radioNum = lib.radioNum;
                    favCollectNum = lib.favCollectNum;
                    return lib;
                }
        };		
        bool Login(const char *account, const char*passwd);
        bool GetMusicLibInfo(MusicLib& lib);
        SongInfo *get_rnd_list(int &result); 
        // @category --- main bill index, range 0-2
        // @sub_bill  --- sub bill index, range 0
        // return  --- return the a  the song list pointer, such as the rnd list
        SongInfo  * get_bills_list(int category, int sub_bill, int &result);
        SongInfo  * get_radio_list(int channel, int index, int &result);

        SongInfo  *get_my_songs(int page, unsigned int &cnt);
        AlbumInfo *get_my_albums(int page, unsigned int &cnt);
        CollectInfo*get_my_collects(int page, unsigned int &cnt);
        CollectInfo*get_my_fav_collects(int page, unsigned int &cnt);
        ArtistInfo *get_my_artists(int page, unsigned int &cnt);
        
        SongInfo *get_albums_songs(unsigned int albums_id, unsigned int page, unsigned int &cnt);
        AlbumInfo *get_artist_albums(unsigned int artist_id, unsigned int page, unsigned int &cnt,unsigned int &total_albums);
        // search songs by key word
        //@ key_word    key word you want to search
        //@page      the return result's page number
        //@cnt    the song's count searched
        SongInfo *search_songs(const char *pKeyWord, unsigned int page, unsigned int &cnt);
        //@id   song, album artist's ID
        // type  COLLECT_TYPE_SONG ..., plse refer the enum
        bool collect_to_my_xiami(unsigned int id, int type, bool collect);
        // for download  files from url
        bool downUrl( const char *pFileName,  const char *pUrl);
        bool isMyFavSong(unsigned int song_id);
};

#define XIAMI_USER_ID "zhanjianbin@snda.com"
#define XIAMI_PWD	   "123456"

#endif

