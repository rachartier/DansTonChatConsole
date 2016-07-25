#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <time.h>

#include <curl/curl.h>

struct s_chunk {
    char    *memory;
    int     size;
};

struct s_param {
    bool random;
    bool last;

    int  n_quotes;
    int  last_quote_id;
    int  quote_selected;
};

typedef struct s_chunk t_chunk;
typedef struct s_param t_param;

int WriteInMemory(void *contents, size_t size, size_t nmemb, void *userp) {
    int         realsize = size * nmemb;
    t_chunk     *chunk = (t_chunk *)userp;

    chunk->memory = realloc(chunk->memory, chunk->size + realsize + 1);

    if(chunk->memory == NULL) {
        fprintf(stderr, "Pas assez de mémoire.\n");

        return 0;
    }

    memcpy(&(chunk->memory[chunk->size]), contents, realsize);
    chunk->size += realsize;
    chunk->memory[chunk->size] = 0;

    return realsize;
}

int GetWordPosition(t_chunk chunk, const char *word) {
    bool word_found = false;

    for(int i = 0; i < chunk.size - (int)strlen(word); ++i) {
        if(chunk.memory[i] == word[0]) {
            int j;
            for(j = 0; j < (int)strlen(word); ++j) {            
                if(chunk.memory[j + i] != word[j]) {
                    word_found = false;
                    i += j;
                    break;                  
                }
                else {
                    word_found = true;        
                }  
            }
            if(word_found)
                return i;
        }
    }
    return 0;
}

int GetLastQuoteId(void) {
    CURL        *curl_handle = NULL;
    CURLcode    res;

    t_chunk     chunk;

    int         j = 0;
    int         w_index = 0;
    int         ret = 0;

    char        digits[16] = {'\0'};

    chunk.memory = malloc(1);
    chunk.size = 0;
   
    curl_handle = curl_easy_init();

    curl_easy_setopt(curl_handle, CURLOPT_URL, "http://danstonchat.com");
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteInMemory);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

    res = curl_easy_perform(curl_handle);
    
    if(res == CURLE_OK) {
        w_index = GetWordPosition(chunk, "item item");   

        if(w_index != 0) {
            for(int i = w_index + (int)strlen("item item"); isdigit(chunk.memory[i]); ++i) {
                digits[j++] = chunk.memory[i];        
            }
        }    
        ret = atoi(digits);
    }

    free(chunk.memory);
    curl_easy_cleanup(curl_handle);
 
    return ret;
}


void EreaseHtmlInQuote(char *arr, int quote_lenght) {
    static const int    n_keywords = 3;
    static const int    n_punctutations = 4;

    static char         *delimiter = "<>#&;";

    static const char   *keyword[] = {
        "/span",
        "br /",
        "span class=\"decoration\"",

        "gt",
        "lt",
        "039",
        "quot"
    };
    static const char *punctuation_translated[] = {
        ">",
        "<",
        "'",
        "\""
    };

    int     new_quote_lenght = 0;

    
    char    *pch = NULL; 
    char    tmp_arr[quote_lenght]; 

    bool    success = true;
   
    memset(tmp_arr, 0, quote_lenght);

    pch = strtok(arr, delimiter);

    while(pch != NULL) { 
        for(int i = 0; i < n_keywords + n_punctutations && success; ++i) {   
            if(strcmp(pch, keyword[i]) == 0) {
                success = false;

                if(i == 1)          strcat(tmp_arr, "\n");
                if(i >= n_keywords) strcat(tmp_arr, punctuation_translated[i - n_keywords]);
            }
        }        
        
        if(success) strcat(tmp_arr, pch);            
       else
           success = true;

        pch = strtok(NULL, delimiter);
    }   

    new_quote_lenght = strlen(tmp_arr);

    memset(arr, '\0', quote_lenght);
    memcpy(arr, tmp_arr, new_quote_lenght);
}

void ParseQuote(char *arr, int quote_lenght) {
    EreaseHtmlInQuote(arr, quote_lenght);            
}

void PrintQuote(char *arr, int quote_id) {
    printf("### %d ###\n", quote_id);
    printf("%s\n\n", arr);
}


void SetUrlDTCQuote(char *url, int quote_id) { 
    char        tmp_url[128] = "http://danstonchat.com/";
    char        quote_id_text[16] = {'\0'};
    
    sprintf(quote_id_text, "%d", quote_id);

    strcat(tmp_url, quote_id_text); 
    strcat(tmp_url, ".html");

    memcpy(url, tmp_url, strlen(tmp_url) + 1);
}

void ShowQuoteText(int quote_id, int res, t_chunk chunk) {
    if(res == CURLE_OK) { 
        int  size_word = (int)strlen("class=\"decoration\">");        
        int  quote_lenght;

        int  quote_begin_index = GetWordPosition(chunk, "class=\"decoration\">") + size_word;
        int  quote_end_index = GetWordPosition(chunk, "</a></p>") ;

        quote_lenght = quote_end_index - quote_begin_index;

        char quote_text[quote_lenght + 1];

        for(int i = 0; i < quote_lenght; ++i) {
            quote_text[i] = chunk.memory[quote_begin_index + i]; 
        }        
        
        quote_text[quote_lenght] = '\0'; 

        ParseQuote(quote_text, quote_lenght);
        PrintQuote(quote_text, quote_id); 
    }
    else {
        fprintf(stderr, "Impossible de se connecter");
    } 
}

void ShowQuote(t_param p) {    
    CURL        *curl_handle = NULL;
    CURLcode     res;

    t_chunk     chunk;

    int         quote_id = 0;
    char        url[128];

    for(int i = 0; i < p.n_quotes; ++i) { 
        chunk.memory = malloc(1);
        chunk.size = 0;
        
        if(p.last)                      quote_id = p.last_quote_id - i;
        else if(p.quote_selected!=-1)   quote_id = p.quote_selected;
        else                            quote_id = rand() % (p.last_quote_id + 1);        
            
        SetUrlDTCQuote(url, quote_id);

        curl_handle = curl_easy_init();
        curl_easy_setopt(curl_handle, CURLOPT_URL, url);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteInMemory);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
        res = curl_easy_perform(curl_handle);
          
        ShowQuoteText(quote_id, res, chunk);
        
        curl_easy_cleanup(curl_handle);
        free(chunk.memory);   
    }
}

int GetNumberThirdArgument(int argc, char *argv[]) {
    if(argc > 2) {
        return atoi(argv[2]);       
    }
    return 1;
}

void ShowHelp(void) {
    static char help_text[] = {
        "Arguments:\n"
            "\tN n'est pas obligé d'etre spécifié\n"
            "\t[-r -random] N: affiche N quotes\n"
            "\t[-l -last] N: affiche les N dernières quotes sorties\n"
            "\t[-q -quote] ID: affiche la quote attribuée à l'ID\n\n"
    };

    printf("%s", help_text);
}

int main(int argc, char *argv[]) {
    curl_global_init(CURL_GLOBAL_ALL);
    srand(time(NULL));
    
    t_param     p;
    
    int last_id_quote = GetLastQuoteId();
        
    p.last_quote_id = last_id_quote;
    p.last = false;
    p.random = false;
    p.quote_selected = -1;
    
    if(last_id_quote == 0) {
        printf("Problème de connection.\n");
        curl_global_cleanup();
        return -1;
    }

    if(argc > 1) {
        int         n_quotes = GetNumberThirdArgument(argc, argv);

        p.n_quotes = n_quotes;
        
        if(strcmp(argv[1], "-r") == 0
        || strcmp(argv[1], "-random") == 0) {
            p.random = true;            
        }
        else if(strcmp(argv[1], "-l") == 0
        ||      strcmp(argv[1], "-last") == 0) {
            p.last = true;   
        }
        else if(strcmp(argv[1], "-q") == 0
        ||      strcmp(argv[1], "-quote") == 0) {
            p.n_quotes = 1;
            p.quote_selected = GetNumberThirdArgument(argc, argv); 
        }
        else if(strcmp(argv[1], "-h") == 0
            ||  strcmp(argv[1], "-help") == 0) {
            ShowHelp();
        }
        else {
            printf("Commande inconnue. Tapez -help pour avoir l'aide.\n");  
        }
        ShowQuote(p);
    }
    else {
        p.random = true;
        ShowQuote(p);
    } 
    
    curl_global_cleanup();
    
    return EXIT_SUCCESS;        
}
