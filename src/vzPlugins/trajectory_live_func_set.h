#ifndef TRAJECTORY_LIVE_FUNC_SET_H
#define TRAJECTORY_LIVE_FUNC_SET_H

struct tlf_set_context
{
    double value;
};

static int tlf_set_init(void** context, char* params, float* value)
{
    struct tlf_set_context* ctx;

    if(!context) return -1;

    ctx = (struct tlf_set_context*)malloc(sizeof(struct tlf_set_context));

    ctx->value = atof(params);

    /* setup context */
    *context = ctx;

    return 0;
};

static int tlf_set_destroy(void** context)
{
    struct tlf_set_context* ctx;

    if(!context) return -1;

    ctx = (struct tlf_set_context*)*context;
    *context = NULL;

    free(ctx);

    return 0;
};

static int tlf_set_calc(void* context, float* value)
{
    struct tlf_set_context* ctx = (struct tlf_set_context*)context;

    if(!ctx) return -1;

    *value = ctx->value;

    return 0;
};

static int tlf_set_dur(void* context)
{
    return 0;
};

#endif /* TRAJECTORY_LIVE_FUNC_SET_H */
