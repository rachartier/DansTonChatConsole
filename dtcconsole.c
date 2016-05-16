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

typedef struct s_chunk t_chunk;

int WriteInMemory(void *contents, size_t size, size_t nmemb, void *userp) {
    int      realsize = size * nmemb;
    t_chunk    *chunk = (t_chunk *)userp;

    chunk->memory = realloc(chunk->memory, chunk->size + realsize + 1);

    if(chunk->memory == NULL) {
        fprintf(stderr, "Not enough memory\n");

        return 0;
    }

    memcpy(&(chunk->memory[chunk->size]), contents, realsize);
    chunk->size += realsize;
    chunk->memory[chunk->size] = 0;

    return realsize;
}

int GetWordPosition(t_chunk chunk, const char *word) {
    bool word_found = false;

    for(int i = 0; i < chunk.size; ++i) {
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
        }
        if(word_found)
            return i;
    }
    return 0;
}

int GetLastQuoteId(t_chunk chunk) {
    int         j = 0;
    int         w_index = 0;
    char        digits[16] = {'\0'};

    w_index = GetWordPosition(chunk, "item item");   

    if(w_index != 0) {
        for(int i = w_index + (int)strlen("item item"); isdigit(chunk.memory[i]); ++i) {
            digits[j++] = chunk.memory[i];        
        }
    }
    return atoi(digits);
}

void EreaseHtmlInQuote(char *arr, int quote_lenght) {

    static const int   n_keywords = 3;
    static const int   n_punctutations = 3;

    static char *delimiter = {"<>#&;"};

    static const char *keyword[] = {
        "/span",
        "br /",
        "span class=\"decoration\"",

        "gt",
        "lt",
        "039"
    };
    static const char *punctuation_translated[] = {
        ">",
        "<",
        "'"
    };

    bool success = true;
    bool new_line = false;

    char *pch = NULL;
    char *tmp_arr = malloc(quote_lenght + 1);

    memset(tmp_arr, 0, quote_lenght);

    pch = strtok(arr, delimiter);

    while(pch != NULL) {
        for(int i = 0; i < n_keywords; ++i) {
            if(strcmp(pch, keyword[i]) == 0) {
                success = false;
                if(i == 1)
                    new_line = true;
            }
        }
        for(int i = n_keywords; i < n_punctutations + n_keywords; ++i) {
            if(strcmp(pch, keyword[i]) == 0) {
                success = false;
                strcat(tmp_arr, punctuation_translated[i - n_keywords]);
            } 
        }
        if(success) {
            if(new_line) {
                strcat(tmp_arr, "\n");
                new_line = false;
            }            

            strcat(tmp_arr, pch);            
        }
        else
            success = true;

        pch = strtok(NULL, delimiter);
    }   

    strcat(tmp_arr, "\n\n\0");

    memset(arr, 0, quote_lenght);
    for(int i = 0; tmp_arr[i] != '\0'; ++i) arr[i] = tmp_arr[i];

    free(tmp_arr);
}


void ParseQuote(char *arr, int quote_lenght) {
    EreaseHtmlInQuote(arr, quote_lenght);            
}

void PrintQuote(char *arr, int quote_id) {
    printf("### %d ###\n\n", quote_id);
    printf("%s\n", arr);
}

void ShowQuote(int quote_id) {
    CURL        *curl_handle = NULL;
    CURLcode     res;

    t_chunk     chunk;

    char        url[128] = "http://danstonchat.com/";
    char        quote_id_text[16] = {'\0'};

    chunk.memory = malloc(1);
    chunk.size = 0;

    sprintf(quote_id_text, "%d", quote_id);

    strcat(url, quote_id_text); 
    strcat(url, ".html");

    curl_handle = curl_easy_init();

    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteInMemory);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

    res = curl_easy_perform(curl_handle);

    if(res == CURLE_OK) { 
        char    *quote_text = NULL;

        int  size_word = (int)strlen("class=\"decoration\">");        
        int  quote_lenght;

        int     quote_begin_index = GetWordPosition(chunk, "class=\"decoration\">") + size_word;
        int     quote_end_index = GetWordPosition(chunk, "</a></p>");

        quote_lenght = quote_end_index - quote_begin_index;
        quote_text = malloc(quote_lenght * sizeof(char));

        for(int i = 0; i < quote_lenght; ++i) {
            quote_text[i] = chunk.memory[quote_begin_index + i]; 
        }        

        ParseQuote(quote_text, quote_lenght);
        PrintQuote(quote_text, quote_id);

        curl_easy_cleanup(curl_handle);
        free(quote_text);
        free(chunk.memory); 
    }
    else {
        fprintf(stderr, "Impossible de se connecter");
    }
}

int GetNumberThirdArgument(int argc, char *argv[]) {
    if(argc > 2) {
        return atoi(argv[2]);       
    }
    return 1;
}

int main(int argc, char *argv[]) {

    CURL        *curl_handle = NULL;
    CURLcode    res;

    t_chunk    chunk;

    chunk.memory = malloc(1);
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_ALL);

    curl_handle = curl_easy_init();


    if(curl_handle) {
        srand(time(NULL));
        curl_easy_setopt(curl_handle, CURLOPT_URL, "http://danstonchat.com");
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteInMemory);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl_handle);

        if(res != CURLE_OK) {
            fprintf(stderr, "Impossible de se connecter.\n");
        }
        else {
            int last_id_quote = GetLastQuoteId(chunk);
            if(argc > 1) {
                if(strcmp(argv[1], "-r") == 0
                        || strcmp(argv[1], "-random") == 0) {
                    int rand_id_quote = rand() % (last_id_quote + 1);

                    for(int i = 0; i < GetNumberThirdArgument(argc, argv); ++i) {
                        rand_id_quote = rand() % (last_id_quote + 1);        
                        ShowQuote(rand_id_quote);
                    }                  

                }
                else if(strcmp(argv[1], "-l") == 0
                ||      strcmp(argv[1], "-last") == 0) {
                    for(int i = 0; i < GetNumberThirdArgument(argc, argv); ++i)
                        ShowQuote(last_id_quote - i);
                }
                else if(strcmp(argv[1], "-q") == 0
                ||      strcmp(argv[1], "-quote") == 0) {
                    int quote_id = GetNumberThirdArgument(argc, argv);

                    if(quote_id > 0 && quote_id <= last_id_quote) {
                        ShowQuote(quote_id);        
                    } else {
                        fprintf(stderr, "La quote n'existe pas.\n");        
                    }
                }
                else {
                    fprintf(stderr,"Commande inconnue.\n");    
                }
            }
            else
                ShowQuote(last_id_quote);
        }

        curl_easy_cleanup(curl_handle);
        free(chunk.memory);

        curl_global_cleanup();
    } 

    return EXIT_SUCCESS;        
}
