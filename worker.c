#include "worker.h"
#include "connect.h"
#include "protocol.h"
#include "log.h"
#include "reflection.h"
#include "main.h"
#include "slre.h"
#include "utils.h"

// 特殊情况特殊处理，后期改造 ，nsuse
typedef struct {
    char *key;
    char *value;
}NsuseImport;

typedef struct {
    char *namespace;
    NsuseImport *imports;
    int imports_size;
}NsuseRst;

static NsuseRst *phpcd_nsuse_new();
static void phpcd_nsuse_destroy(NsuseRst *n);
static int phpcd_nsuse_add_imports(NsuseRst *n, char *import_key, char *import_val);

static int phpcd_get_channel_id(Protocol *p)
{
    int sockfd = p->sockfd;
    int ret;
    int size;
    char *pack = NULL;
    char *method = "vim_get_api_info";
    if (!sockfd) {
        phpcd_log(PHPCD_LOG_ERROR,
                "Invalid sockfd(%d)", sockfd);
        exit(1);
    }

    protocol_call_rpc_req(p, 0, method, NULL);
    pack = recv_pack(p, &size);

    msgpack_zone mempool;
    msgpack_zone_init(&mempool, 2048);
    msgpack_object deserialized;

    ret = msgpack_unpack(
            pack,
            size,
            NULL,
            &mempool,
            &deserialized
    );
    if (MSGPACK_UNPACK_SUCCESS != ret) {
        phpcd_log(PHPCD_LOG_ERROR, "Failed to get channel_id");
        exit(1);
    }
    free(pack);
    return deserialized.via.array.ptr[4].via.i64;
}

static void phpcd_set_channel_id(Protocol *p, int channel_id)
{
    char cmd[32] = { 0 };
    char *method = "vim_command";
    sprintf(cmd, "let g:phpcd_channel_id = %d", channel_id);

    RpcReqParams *params = rpc_req_params_new();
    rpc_req_params_add_str(params, cmd);

    // call rpc requst
    protocol_call_rpc_req(p, 0, method, params);

    rpc_req_params_destroy(params);
}

static int phpcd_nsuse(char *filename, NsuseRst *nsuse)
{
    nsuse->namespace = NULL;
    nsuse->imports = NULL;
    if (FALSE == check_file_exists(filename)) {
        LOG_WARN("filename:%s not found", filename);
        return -1;
    }
    // read file
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        LOG_WARN("Can not open file :%s", filename);
        return -1;
    }
    char buff[1024] = { 0 };
    while (fgets(buff, 1024, fp)) {
        struct slre_cap caps[3];
        if (slre_match("(class|interface|trait)\\s+(\\S+)",
                    buff, strlen(buff), caps, 3, 0) > 0) {
            break;
        }
        char *trimed = trim(buff);
        if (strlen(trimed) == 0) {
            continue;
        }
        if (slre_match("<\\?php?\\s*namespace\\s+(.*);$",
                    buff, strlen(buff), caps, 3, 0) > 0) {
            nsuse->namespace = strndup(caps[0].ptr, caps[0].len);
        } else {
            char *buff_tmp = strdup(buff);
            buff_tmp = substr(buff_tmp, 0, 3);
            if (strcmp(buff_tmp, "use") == 0) {
                int pos = strripos(buff, " as ");
                if (-1 != pos) {
                    char *alias = buff;
                    alias = substr(buff, pos + 3, -1);
                    alias = trim(alias);
                    char *value = trim(substr(buff, 3, pos - 3));
                    phpcd_nsuse_add_imports(nsuse, alias, value);
                } else {
                    int slash_pos = strripos(buff, "\\");
                    char *alias = buff;
                    if (-1 == slash_pos) {
                        alias = substr(buff, 4, -1);
                        alias = trim(alias);
                    } else {
                        alias = substr(buff, slash_pos + 1, -1);
                        alias = trim(alias);
                    }
                }
            }
            free(buff_tmp);
        }
    }
    fclose(fp);
    return 0;
}

static NsuseRst *phpcd_nsuse_new()
{
    NsuseRst *n = (NsuseRst *)malloc(sizeof(NsuseRst));
    n->namespace = NULL;
    n->imports = NULL;
    n->imports_size =  0;
    return n;
}

static void phpcd_nsuse_destroy(NsuseRst *n)
{
    if (n->namespace) {
        free(n->namespace);
    }
    if (n->imports_size > 0) {
        int i = 0;
        for (; i < n->imports_size; i++) {
            free(n->imports[i].key);
            free(n->imports[i].value);
        }
        free(n->imports);
    }
    free(n);
}

static int phpcd_nsuse_add_imports(NsuseRst *n, char *import_key, char *import_val)
{
    if (n->imports_size == 0) {
        n->imports = (NsuseImport *)malloc(sizeof(NsuseImport));
    } else {
        n->imports = (NsuseImport *)realloc(n->imports, sizeof(NsuseImport) * (n->imports_size + 1));
    }
    n->imports[n->imports_size].key = strdup(import_key);
    n->imports[n->imports_size].value = strdup(import_key);
    n->imports_size++;
    return 0;
}

static void phpcd_do_work(Protocol *p, RpcResponse *response)
{
    if (response->type == RESPONSE_TYPE_NORMAL) {
        char *method = PHPCD_METHOD_NORMAL(response);
        if (strcmp(method, "location") == 0) {

            RflRstLocation *r = rfl_rst_location_new();

            char *class_name = rpc_response_params_str(response, 0);
            char *method_name = rpc_response_params_str(response, 1);
            if (strlen(class_name) && strlen(method_name)) {
                // location class::method
                rfl_location_method(class_name, method_name, r);
                LOG_DEBUG("Location class, class_name:%s, method_name:%s",
                        class_name, method_name);
            } else if (strlen(method_name)){
                // location function
                rfl_location_func(method_name , r);
                LOG_DEBUG("Location function, function_name:%s", method_name);
            } else {
                // location class
                rfl_location_class(class_name, r);
                LOG_DEBUG("Location class, class_name:%s", class_name);
            }
            free(class_name);
            free(method_name);

            if (r->result == 1) {
                RpcReqParams *params = rpc_req_params_new();
                rpc_req_params_add_str(params, r->filename);
                rpc_req_params_add_int(params, r->start_line);
                protocol_call_rpc_rsp(
                        p,
                        PHPCD_MSGID(response),
                        NULL,
                        params
                );
                rpc_req_params_destroy(params);
            }
            rfl_rst_location_destroy(r);
        } else if (strcmp(method, "nsuse") == 0) {
            NsuseRst *nsuse = phpcd_nsuse_new();
            char *file_path = rpc_response_params_str(response, 0);
            if (-1 != phpcd_nsuse(file_path, nsuse)) {
                msgpack_packer *pk = NULL;
                msgpack_sbuffer *sbuf = (msgpack_sbuffer *)malloc(sizeof(msgpack_sbuffer));
                msgpack_sbuffer_init(sbuf);
                msgpack_packer_init(pk, sbuf, msgpack_sbuffer_write);
                // init pack
                msgpack_pack_array(pk, 4);
                // rpc type
                msgpack_pack_int(pk, 1);
                // rpc msgid
                msgpack_pack_int(pk, PHPCD_MSGID(response));

                //rpc error
                msgpack_pack_nil(pk);

                // rpc params
                msgpack_pack_map(pk, 2);

                // key
                msgpack_pack_str(pk, strlen("namespace"));
                msgpack_pack_str_body(pk, "namespace", strlen("namespace"));
                // value
                msgpack_pack_str(pk, strlen(nsuse->namespace));
                msgpack_pack_str_body(pk, nsuse->namespace, strlen(nsuse->namespace));

                // key
                msgpack_pack_str(pk, strlen("imports"));
                msgpack_pack_str_body(pk, "imports", strlen("imports"));
                // value
                msgpack_pack_map(pk, nsuse->imports_size);

                int i = 0;
                for (; i < nsuse->imports_size; i++) {
                    // key
                    msgpack_pack_str(pk, strlen(nsuse->imports[i].key));
                    msgpack_pack_str_body(pk, nsuse->imports[i].key, strlen(nsuse->imports[i].key));
                    // value
                    msgpack_pack_str(pk, strlen(nsuse->imports[i].value));
                    msgpack_pack_str_body(pk, nsuse->imports[i].value, strlen(nsuse->imports[i].value));
                }

                // Send response
                protocol_call_rpc_raw(p, (void *)sbuf->data, sbuf->size);
                msgpack_packer_free(pk);
            }
            free(file_path);
            phpcd_nsuse_destroy(nsuse);
        }
    } else if (response->type == RESPONSE_TYPE_NOTIFY) {
        // todo 暂时未用到
    }
}

int phpcd_loop(Protocol *p)
{
    int channel_id = phpcd_get_channel_id(p);
    phpcd_set_channel_id(p, channel_id);

    int size, ret;
    char *pack = NULL;
    while (NULL != (pack = recv_pack(p, &size))) {
        RpcResponse *response = rpc_response_new();
        ret = protocol_unpack(pack, size, response);
        msgpack_data_print(pack, size, NULL);
        if (-1 == ret) {
            continue;
        }
        phpcd_do_work(p, response);
        free(pack);
        pack = NULL;
        rpc_response_destroy(response);
    }
    return 0;
}
