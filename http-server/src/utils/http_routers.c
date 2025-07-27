#include "../headers/http_routers.h"


void http_rts_list_init(struct http_rts_list* list)
{
        list->head = NULL;
}


int http_rts_list_add(struct http_rts_list* list, struct http_route rt)
{
        struct http_rts_lnode** cur = &(list->head);
        while (*cur) {
                cur = &((*cur)->next);
        }

        *cur = (struct http_rts_lnode*) calloc(1,
            sizeof(struct http_rts_lnode));
        if (!*cur) {
                fprintf(stderr, "No memory\n");
                return EXIT_FAILURE;
        }

        (*cur)->rt = rt;
        (*cur)->next = NULL;

        return EXIT_SUCCESS;
}


void http_rts_list_free(struct http_rts_list* list)
{
        struct http_rts_lnode* cur = list->head;
        while (cur) {
                void* prev = cur;
                cur = cur->next;
                free(prev);
        }
}


struct http_rts_list* get_http_router()
{
        static struct http_rts_list router = { 0 };
        return &router;
}


int set_http_route(struct http_route rt)
{
        struct http_rts_list* router = get_http_router();
        return http_rts_list_add(router, rt);
}


struct http_route* get_http_route(const char* route)
{
        struct http_rts_list* router = get_http_router();

        struct http_rts_lnode* cur = router->head;
        while (cur) {
                if (!strcmp(cur->rt.route, route)) return &(cur->rt);
                cur = cur->next;
        }

        return NULL;
}
