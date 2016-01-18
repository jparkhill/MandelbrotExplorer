/********************************************************************************
 *                                                                              *
 * Mandelbrot Explorer               JAP 2016
 *                                                                              *
 ********************************************************************************/

#include "Mandel.hpp"

using namespace std;

void UpdateTexture(SDL_Texture *texture)
{
    done = SDL_FALSE;
    loops=0;

    SDL_PixelFormat fmt;
    Uint32 *dst;
    int row, col;
    void *pixels;
    int pitch;

    if (SDL_LockTexture(texture, NULL, &pixels, &pitch) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't lock texture: %s\n", SDL_GetError());
        throw;
    }
    
    // Compute the mandlebrot set.
    double MBSetf[Width*Height]; // 11 bits of exponent and 52 of fraction (1 of sign)
    Uint8 MBSeti[Width*Height];
    Mandel(MBSetf);
    // Map the sets values onto integers.
    double mx = 0.0;
    for (int i=0; i<Width*Height; ++i)
    {
        if (MBSetf[i]>mx)
            mx = MBSetf[i];
    }
    for (int i=0; i<Width*Height; ++i)
        MBSeti[i] = (Uint8)(UINT8_MAX*(MBSetf[i]/mx));

    for (row = 0; row < Height; ++row)
    {
        dst = (Uint32*)((Uint8*)pixels + row * pitch);
        for (col = 0; col < Width; ++col)
        {
//            bitmask_t color[3];
//            hilbert_i2c( 3, 8, (MBSeti[row*Width+col]+loops*100) , color);
            *dst++ = (  ((Uint32)MBSeti[row*Width+col] << 16) | ((Uint32)(loops%255) << 0)); // | ((Uint32)color[1] << 8)
        }
    }
 
    SDL_UnlockTexture(texture);
}

/*

// Render two textures.
void renderTextures(SDL_Texture *t1, SDL_Texture *t2, SDL_Renderer *ren, int x, int y, SDL_Rect *clip = nullptr){
    SDL_Rect dst;
    dst.x = x;
    dst.y = y;
    if (clip != nullptr){
        dst.w = clip->w;
        dst.h = clip->h;
    }
    else {
        SDL_QueryTexture(t1, NULL, NULL, &dst.w, &dst.h);
    }
    renderTexture(t1, ren, dst, clip);
}

void loop()
{
    cout << " loops " << loops << endl;
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_ESCAPE) {
                done = SDL_TRUE;
            }
            break;
        case SDL_QUIT:
            done = SDL_TRUE;
            break;
        }
    }
    
//    UpdateTexture(texture);
    SDL_RenderClear(renderer);
    SDL_Color color = { 255, 255, 255, 255 };
    SDL_Surface* surf;
    
    string ls=std::to_string(loops);
    renderText(surf, "Loop: "+ls, color);
    // Seems like surf is garbage?
    cout << "Surf" << surf->w << " " << surf->h << " " << surf->pitch << endl;
    
    SDL_Texture *text_text = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);
    
    int iW, iH;
    SDL_QueryTexture(text_text, NULL, NULL, &iW, &iH);
    cout << "x:" << iW << " y:" << iH << endl;
    int x = Width / 2 - iW / 2;
    int y = Height / 2 - iH / 2;
    renderTextures(text_text, texture, renderer, x, y);
    
    SDL_RenderPresent(renderer);
    SDL_DestroyTexture(text_text);
    
    loops++;

}
*/


int main(int argc, char **argv)
{
    GApp App;
    App.Loop();
    exit(0);
    return 0;
}

