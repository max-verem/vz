#ifndef TRAJECTORY_LIVE_FUNC_SEQ_H
#define TRAJECTORY_LIVE_FUNC_SEQ_H

struct tlf_seq_context
{
    int func_cnt;
    char** func_list;
    int func_idx;
    struct
    {
        int cnt;
        void* ctx;
        struct trajectory_live_func* func;
    } current;
};

static int tlf_seq_init(void** context, char* params, float* value)
{
    struct tlf_seq_context* ctx;

    if(!context) return -1;

    ctx = (struct tlf_seq_context*)malloc(sizeof(struct tlf_seq_context));
    memset(ctx, 0, sizeof(struct tlf_seq_context));

    /* parse functions list */
    ctx->func_cnt = split_str(params, ":", &ctx->func_list);

    if(!ctx->func_cnt)
    {
        logger_printf(1, "tlf_seq_init: FAILED TO PARSE params=[%s]", params);

        if(ctx->func_list)
            split_str_free(&ctx->func_list);

        free(ctx);

        return -2;
    };

    /* setup context */
    *context = ctx;

    return 0;
};

static int tlf_seq_destroy(void** context)
{
    struct tlf_seq_context* ctx;

    if(!context) return -1;

    ctx = (struct tlf_seq_context*)*context;
    *context = NULL;

    if(!ctx) return -1;

    if(ctx->current.func)
        ctx->current.func->destroy(&ctx->current.ctx);

    if(ctx->func_list)
        split_str_free(&ctx->func_list);

    free(ctx);

    return 0;
};

static int tlf_seq_calc(void* context, float* value)
{
    struct tlf_seq_context* ctx = (struct tlf_seq_context*)context;

    if(!ctx) return -1;

    /* check if out of subsequence */
    if(ctx->current.func && ctx->current.func->dur(ctx->current.ctx) <= ctx->current.cnt)
    {
        ctx->current.func->destroy(&ctx->current.ctx);
        ctx->current.func = NULL;
    };

    /* check if played sequence */
    if(!ctx->current.func && ctx->func_idx >= ctx->func_cnt)
        return -2;

    /* check if need to init func */
    if(!ctx->current.func)
    {
        int l;

        /* lookup for function */
        ctx->current.func = trajectory_live_func_lookup(ctx->func_list[ctx->func_idx], &l);

        /* check if function found */
        if(ctx->current.func)
        {
            /* trim last quote */
            int l2 = strlen(ctx->func_list[ctx->func_idx]);
            if(')' == ctx->func_list[ctx->func_idx][l2 - 1])
            {
                ctx->func_list[ctx->func_idx][l2 - 1] = 0; l2--;
            };

            /* try to init */
            if(ctx->current.func->init(&ctx->current.ctx, ctx->func_list[ctx->func_idx] + l + 1, value))
                ctx->current.func = NULL;
        };

        ctx->func_idx++;
        ctx->current.cnt = 0;
    };

    ctx->current.cnt++;

    /* check if function present */
    return (ctx->current.func)?ctx->current.func->calc(ctx->current.ctx, value):-1;
};

static int tlf_seq_dur(void* context)
{
    return 0;
};

#endif /* TRAJECTORY_LIVE_FUNC_LINETO_H */
