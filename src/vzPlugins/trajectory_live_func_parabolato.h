#ifndef TRAJECTORY_LIVE_FUNC_PARABOLATO_H
#define TRAJECTORY_LIVE_FUNC_PARABOLATO_H

struct tlf_parabolato_context
{
    double A;
    double B;
    int dur;
    int type;
    int cnt;
    double a;
    double b;
};

static int tlf_parabolato_init(void** context, char* params, float* value)
{
    struct tlf_parabolato_context* ctx;

    if(!context) return -1;

    ctx = (struct tlf_parabolato_context*)malloc(sizeof(struct tlf_parabolato_context));

    /* scan params */
    if(3 != sscanf(params, "%lf,%d,%d", &ctx->B, &ctx->dur, &ctx->type) || !ctx->dur)
    {
        free(ctx);
        logger_printf(1, "tlf_parabolato_init: FAILED TO PARSE params=[%s]", params);
        return -2;
    };

    /* set additional params */
    ctx->cnt = 0;
    ctx->A = *value;

    /* params case */
    switch(ctx->type)
    {
        case 1:
            ctx->a = (ctx->B - ctx->A) / (ctx->dur * ctx->dur);
            break;
        case 3:
            ctx->a = (ctx->A - ctx->B) / (ctx->dur * ctx->dur);
            break;
        default:
            free(ctx);
            logger_printf(1, "tlf_parabolato_init: UNKNOWN type=[%d], params=[%s]",
                ctx->type, params);
            return -2;
    };

    /* setup context */
    *context = ctx;

    return 0;
};

static int tlf_parabolato_destroy(void** context)
{
    if(!context) return -1;
    if(!(*context)) return -1;

    free(*context);
    *context = NULL;

    return 0;
};

static int tlf_parabolato_calc(void* context, float* value)
{
    struct tlf_parabolato_context* ctx = (struct tlf_parabolato_context*)context;

    if(!ctx) return -1;

    if(ctx->cnt <= ctx->dur)
    {
        switch(ctx->type)
        {
            case 1:
                *value = ctx->A + ctx->a * ctx->cnt * ctx->cnt;
                break;
            case 2:
                break;
            case 3:
                *value = ctx->A + ctx->a * ctx->cnt * (ctx->cnt - 2 * ctx->dur);
                break;
            case 4:
                break;
        };

        ctx->cnt++;
    };

    return 0;
};

#endif /* TRAJECTORY_LIVE_FUNC_PARABOLATO_H */
