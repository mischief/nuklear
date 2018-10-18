#pragma lib "libnuklear.a"

NK_API void nk_plan9_makefont(struct nk_user_font *nkfont, Font *f);
NK_API void nk_plan9_render(struct nk_context *ctx, Image *image);
NK_API int nk_plan9_handle_kbd(struct nk_context *ctx, char *kbd, int len);
NK_API int nk_plan9_handle_mouse(struct nk_context *ctx, Mouse m, Point p);
