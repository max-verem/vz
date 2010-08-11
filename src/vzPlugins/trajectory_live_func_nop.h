#ifndef TRAJECTORY_LIVE_FUNC_NOP_H
#define TRAJECTORY_LIVE_FUNC_NOP_H

struct tlf_nop_context
{
    int dur;
    int cnt;
};

static int tlf_nop_init(void** context, char* params, float* value)
{
    struct tlf_nop_context* ctx;

    if(!context) return -1;

    ctx = (struct tlf_nop_context*)malloc(sizeof(struct tlf_nop_context));

    ctx->dur = atol(params);
    ctx->cnt = 0;

    *context = ctx;

    return 0;
};

static int tlf_nop_destroy(void** context)
{
    if(!context) return -1;
    if(!(*context)) return -1;

    free(*context);
    *context = NULL;

    return 0;
};

static int tlf_nop_calc(void* context, float* value)
{
    struct tlf_nop_context* ctx = (struct tlf_nop_context*)context;

    if(!ctx) return -1;

    /* check for out of range */
    if(ctx->cnt <= ctx->dur)
    {
        ctx->cnt++;
        return 0;
    };

    return -2;
};

static int tlf_nop_dur(void* context)
{
    struct tlf_nop_context* ctx = (struct tlf_nop_context*)context;

    if(!ctx) return -1;

    return ctx->dur;
};

#endif /* TRAJECTORY_LIVE_FUNC_NOP_H */
