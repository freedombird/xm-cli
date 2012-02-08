#include <string.h>
#include   <sys/time.h>

#include "data_manager.h"

static void dl_test(DataManager *pManager );

// radio
const char fm_radio_promo[] = "http://api.xiami.com/app/android/radios-promo?page=1";
const char fm_radio_page[] = "http://api.xiami.com/app/android/radios?page=1";

//const char fm_category[] = "http://api.xiami.com/app/android/radio-category";
//const char fm_radio[] = "http://api.xiami.com/app/android/radio?id=%d";
//fm_times
//fm_style
//fm_constellation

// fail
const char fm_radio_top[] ="http://api.xiami.com/app/android/radiostop?page=1";

//am collect faild
const char am_collect_promo[] ="http://api.xiami.com/app/android/collects-promo";
const char am_collect_top[]="http://api.xiami.com/app/android/collectstop?page=1";

// search failed
const char search_singer[]="http://api.xiami.com/app/android/search?key=liu&page=1";

//song info
const char song_info[]="http://api.xiami.com/app/android/song?id=173892";
const char collect_info[]="http://api.xiami.com/app/android/collect?id=274869";

//my musib lib, and required auth with user id
const char my_artist[]="http://api.xiami.com/app/android/lib-artists?uid=4460056";
const char my_songs[]="http://api.xiami.com/app/android/lib-rnd?uid=4460056";
const char my_am[]="http://api.xiami.com/app/android/lib-collects?uid=4460056";

DataManager *manager = new  DataManager();


static void dump_songs(int cnt, SongInfo *pSongs)
{
    int k;
    for(k=0; k<cnt && k<XIAMI_MY_SONGS_PAGE; k++)
    {
        printf("index %d name=%s, location=%s\n",  k, pSongs->name, pSongs->location);
        pSongs++;
    }    
}

static void dump_albums(int cnt, AlbumInfo *pAlbum)
{
    int k;

    printf("albums cnt=%d\n", cnt);
    for(k=0; k<cnt; k++)
    {
        printf("index%d title=%s logo=%s obj_id=%d\n",  k, pAlbum->title, pAlbum->logo, pAlbum->id);
        pAlbum++;
    }
}

static void get_rnd_list(DataManager *pDataManager)
{
    SongInfo *pSongs = NULL;
     int ret;
    int k;
    
    pSongs = pDataManager->get_rnd_list(ret);
        for(k=0; k<20; k++)
        {
            printf("index %d name=%s, location=%s, title=%s\n", k, pSongs->name, 
            pSongs->location, pSongs->album_title);
            pSongs++;
        }
}

static void get_bills_list_test(DataManager *pDataManager)
{
    SongInfo *pSongs = NULL;
     int ret = -1;
    int i =0, j=0, k=0;

    pSongs = pDataManager->get_bills_list(i, j, ret);
    if(ret > 0 && pSongs)
    {
        for(k=0; k<ret; k++)
        {
            printf("index %d name=%s, location=%s\n", k, pSongs->name, pSongs->location);
            pSongs++;
        }        
     }
}

static void get_radio_list_test(DataManager *pDataManager)
{
    SongInfo *pSongs = NULL;
     int ret = -1;
    int i=0, j=0, k=0;

    for (i = 0; i < 2; i++)
    {
                printf("get_radio _list_test i=%d, j=%d\n", i, j);
                pSongs = pDataManager->get_radio_list(j, i, ret);
                if(ret >= 0 && pSongs)
                {
                    for(k=0; k<10; k++)
                    {
                        printf(" i=%d index %d name=%s, location=%s\n", i, k, pSongs->name, pSongs->location);
                        pSongs++;
                    }
                }
                sleep(1);
        }
}

static int tim_subtract(struct timeval *result, struct timeval *x, struct timeval *y)
{
    if ( x->tv_sec > y->tv_sec )
        return   -1;

    if ((x->tv_sec==y->tv_sec) && (x->tv_usec>y->tv_usec))
        return   -1;

    result->tv_sec = ( y->tv_sec-x->tv_sec );
    result->tv_usec = ( y->tv_usec-x->tv_usec );
    if (result->tv_usec <0 )
    {
	result->tv_sec--;
	result->tv_usec += 1000000;
    }
    return   0;
}

static void dl_test(DataManager *pManager )
{
    struct timeval   start, stop, diff;
    int result;
    SongInfo *pSongs = NULL;
    
    printf("*** Get random good songs\n\n");
    gettimeofday(&start,0);
    pManager->get_rnd_list(result);
    gettimeofday(&stop,0);
    tim_subtract(&diff, &start, &stop);
    
    printf("get rnd total cost seconds=%d, %d us\n", (int)diff.tv_sec, (int)diff.tv_usec);

    printf("*** download ... \n");
    
    pSongs = pManager->get_rnd_list(result);
              
    printf("first name=%s, location=%s\n",  pSongs->name, pSongs->location);
    start.tv_sec = 0;
    start.tv_usec = 0;
    gettimeofday(&start,0);
    
    //pManager->downloadSong(pSongs->name, pSongs->location, FILE_TYPE_MP3);
    stop.tv_sec = 0;
    stop.tv_usec = 0;
    gettimeofday(&stop,0);
    tim_subtract(&diff,&start,&stop);
    printf("Downloading.....  total cost %d, %d us\n", (int)diff.tv_sec, (int)diff.tv_usec);
}

#define NEED_LOGIN
//#define XIAMI_GET_MY_LIB
//#define XIAMI_COLLECT_TEST
static void  get_my_fav(DataManager *manager)
{
    int userid = 0;
    std::string nickname;
    std::string msg ;
    DataManager::MusicLib lib;    
    unsigned int cnt = 0;
    SongInfo *pMysongs;
    AlbumInfo *pAlbum;

 #ifdef NEED_LOGIN   
    if (manager->Login(XIAMI_USER_ID, XIAMI_PWD))
    {
#ifdef XIAMI_GET_MY_LIB 
        printf("===========GetMusicLibInfo =============\n");    
        manager->GetMusicLibInfo(lib);            
        printf("lib info: songs=%d, albums=%d artist=%d\n", lib.songNum, lib.albumNum, lib.artistNum);
#endif

#ifdef XIAMI_GET_MY_SONGS
        printf("===========get_my_songs =============\n");
        pMysongs = manager->get_my_songs(1, cnt);
        dump_songs(cnt,  pMysongs);
#endif

#ifdef XIAMI_GET_MY_ALBUMS
        printf("===========get_my_albums =============\n");        
        pAlbum=manager->get_my_albums(0, cnt);
        if(!pAlbum){
            printf("get my album failed\n");
            return;
        }        
        dump_albums(cnt, pAlbum);

        printf("===========get_my_collects =============\n");
        pAlbum = manager->get_my_collects(0, cnt);
        printf("*** get my col cnt=%d::\n", cnt);        
        dump_albums(cnt, pAlbum);

        printf("===========get_my_fav_collects =============\n");
        pAlbum = manager->get_my_fav_collects(0, cnt);
        dump_albums(cnt, pAlbum);

        printf("===========get_my_artists =============\n");
        pAlbum=manager->get_my_artists(0, cnt);
        printf("*** get my artist::\n");        
        dump_albums(cnt, pAlbum);

        printf("===========get_artist_albums =============\n");
        unsigned int artist_id =0;
        artist_id = pAlbum->id;
        printf("*** get my artist id=%d::\n", artist_id);
        unsigned int total=0;
        pAlbum=manager->get_artist_albums(artist_id, 1, cnt, total);
        printf("*** get my artist's albums::cnt=%d, total=%d\n", cnt, total);
        dump_albums(cnt, pAlbum);

        printf("===========get_albums_songs =============\n");
        unsigned int albums_id = 32418;
        albums_id = pAlbum->id;
        printf("*** get   albums_id id=%d::\n", albums_id);        
        pMysongs = manager->get_albums_songs(albums_id, 1, cnt);
        printf("*** get my artist's albums songs::cnt=%d\n", cnt);
        dump_songs(cnt,  pMysongs);
#endif                
    }
#endif // endif NEED_LOGIN    
#ifdef XIAMI_SEARCH_TEST
    printf("===========search_songs =============\n");
    const char key[] ="zxy";
    pMysongs = manager->search_songs(key, (unsigned int)1, cnt);
    
    printf("*** get my artist's albums songs::cnt=%d\n", cnt);
    dump_songs(cnt,  pMysongs);
#endif

#ifdef XIAMI_COLLECT_TEST
        printf("===========collect my songs to xiami.com =============\n");
        unsigned int song_id = 74393; // xueyou's one tear
        int type = COLLECT_TYPE_SONG;
        manager->collect_to_my_xiami(song_id,  type, true);
        
        printf("===========collect artist  =============\n");
        type = COLLECT_TYPE_ARTIST;
        song_id = 78523;
        manager->collect_to_my_xiami(song_id,  type, false);
#endif  // XIAMI_COLLECT_TEST    
        printf("===========isMyFavSong =============\n");
        bool status=false;
        status=manager->isMyFavSong(1);
        if(status)
            printf("*** In my fav list\n");
        else
            printf("*** Not found in my fav list\n");
        
}

int main(int argc, char *argv[])
{
    AlbumInfo albums;
    CollectInfo collects;

#if 0
    get_rnd_list(manager);
    sleep(2);
    printf("get the second one \n");
    get_rnd_list(manager);
    printf("finish \n");
#endif
    //printf("===========get_my_fav =============\n");
    //get_my_fav(manager);

#if 1
//    printf("===========get_rnd_list =============\n");
//    get_rnd_list(manager);

    int i;
    for(i=0; i<10; i++){
        printf("===========get_bills_list_test =============\n");
        get_bills_list_test(manager);
    }
     while(1)
    {
        sleep(1);
    }
#endif 

    delete manager;

    return 0;
}

