#ifndef TRAJECTORY_LIVE_FUNC_LINETO_H
#define TRAJECTORY_LIVE_FUNC_LINETO_H

struct tlf_lineto_context
{
    double A;
    double B;
    double speed;
    int dur;
    int cnt;
};

static int tlf_lineto_init(void** context, char* params, float* value)
{
    struct tlf_lineto_context* ctx;

    if(!context) return -1;

    ctx = (struct tlf_lineto_context*)malloc(sizeof(struct tlf_lineto_context));

    /* scan params */
    if(3 != sscanf(params, "%lf,%lf,%d", &ctx->B, &ctx->speed, &ctx->dur))
    {
        free(ctx);
        logger_printf(1, "tlf_lineto_init: FAILED TO PARSE params=[%s]", params);
        return -2;
    };

    /* set additional params */
    ctx->cnt = 0;
    ctx->A = *value;

    /* setup context */
    *context = ctx;

    /* normalize speed */
    if((ctx->A < ctx->B && ctx->speed < 0) || (ctx->A > ctx->B && ctx->speed > 0))
        ctx->speed *= -1.0;

    return 0;
};

static int tlf_lineto_destroy(void** context)
{
    if(!context) return -1;
    if(!(*context)) return -1;

    free(*context);
    *context = NULL;

    return 0;
};

static int tlf_lineto_calc(void* context, float* value)
{
    struct tlf_lineto_context* ctx = (struct tlf_lineto_context*)context;

    if(!ctx) return -1;

    /* programmed count of fields */
    if(ctx->dur)
    {
        /* check for out of range */
        if(ctx->cnt < ctx->dur)
        {
            /* increment counter */
            ctx->cnt++;

            /* set value */
            *value = ctx->A + ctx->cnt * (ctx->B - ctx->A) / ctx->dur;
        };

        return 0;
    };

    if(!((ctx->speed > 0 && *value > ctx->B) || (ctx->speed < 0 && *value < ctx->B)))
        *value = *value + ctx->speed;

    return 0;
};

static int tlf_lineto_dur(void* context)
{
    struct tlf_lineto_context* ctx = (struct tlf_lineto_context*)context;

    if(!ctx) return -1;

    return ctx->dur;
};

#endif /* TRAJECTORY_LIVE_FUNC_LINETO_H */
