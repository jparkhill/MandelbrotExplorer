#ifndef Mandelh
#define Mandelh

/********************************************************************************
 * Mandelbrot Explorer John Parkhill 2016                                       *
 * Requires sdl2, sdl_image sdl_ttf. (all available in homebrew, apt-get)       *
 * compilation requires CMake                                                   *
 * (On OSX) brew install sdl2  (etc.)                                           *                                    *
 ********************************************************************************/

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <vector>
#include <complex>
#include <fstream>
#include <iostream>

//#include "hilbert.h" // For assigning colors to the Mandlebrot set.

#define NO_STDIO_REDIRECT
#include <SDL.h>
#include <SDL_ttf.h>

#define Width 1024
#define Height 768

// These are the "State Variables"
// which define the lower left corner of the mandlebrot and scale
int loops;
double xp=-1.5;
double yp=-1.0;
double zoom=1.0;

SDL_bool done;
TTF_Font *font;
SDL_Renderer *renderer;
SDL_Window *window;
SDL_RWops *handle;
SDL_Surface* screen;

using namespace std;

std::string getPath(const std::string &subDir = ""){
#ifdef _WIN32
    const char PATH_SEP = '\\';
#else
    const char PATH_SEP = '/';
#endif
    static std::string baseRes;
    if (baseRes.empty()){
        char *basePath = SDL_GetBasePath();
        if (basePath){
            baseRes = basePath;
            SDL_free(basePath);
        }
        else {
            std::cerr << "Error getting resource path: " << SDL_GetError() << std::endl;
            return "";
        }
        //We replace the last bin/ with res/ to get the the resource path
        //        size_t pos = baseRes.rfind("bin");
        //      baseRes = baseRes.substr(0, pos) + "res" + PATH_SEP;
    }
    return subDir.empty() ? baseRes : baseRes + subDir + PATH_SEP;
}

// Generate Width x Height Mandlebrot set.
void Mandel(double* Dest, double x0=-1.5, double y0=-1.0 , double zoom=1.0)// , double x1=0.5, double y1=1.0)
{
    double x1 = (x0 + 2.0)/zoom;
    double y1 = (y0 + 2.0)/zoom;
    int xi=0;
    for (double x=x0; x<x1, xi<Width ; x+=(x1-x0)/Width)
    {
        int yi=0;
        for (double y=y0; y<y1, yi<Height; y+=(y1-y0)/Height)
        {
            complex<double> c(x,y);
            complex<double> z(c);
            for (int i=0;i<80.0;i++)
            {
                z=z*z+c;
                if (abs(z)>5.0)
                {
                    Dest[yi*Width+xi] = abs(z);
                    break;
                }
            }
            Dest[yi*Width+xi]= abs(z);
            yi++;
        }
        xi++;
    }
    return;
}

//
// GObj
//  A graphical object. Data to be shown and the position of that data.
class GObj
{
public:
    SDL_Surface* surf;
    int x0,y0,x1,y1; // Desired corners of the object in the screen.
    ~GObj()
    {
        SDL_FreeSurface(surf);
    }
};

void renderText(SDL_Surface*& surf, const std::string &message, SDL_Color color)
{
    //We need to first render to a surface as that's what TTF_RenderText returns, then
    //load that surface into a texture
    surf = TTF_RenderText_Blended(font, message.c_str(), color);
    if (surf == nullptr){
        TTF_CloseFont(font);
        cout << "Failed to render text." << endl;
        throw;
    }
}

GObj* LoopText()
{
    GObj* tore = new GObj();
    SDL_Color color = { 255, 255, 255, 255 };
    string ls=std::to_string(loops);
    renderText(tore->surf, "Loop: "+ls, color);
    cout << "Surf:" << tore->surf->w << " " << tore->surf->h << " " << tore->surf->pitch << endl;
    return tore;
}

GObj* MandelImg()
{
    GObj* img = new GObj();
    img->surf = SDL_CreateRGBSurface(0,Width,Height,32,0,0,0,0); // 32 bit pixeldepth.
    SDL_LockSurface(img->surf);
    
    SDL_SetSurfaceBlendMode(img->surf, SDL_BLENDMODE_ADD);
    
    Uint32* pixels = reinterpret_cast<Uint32*>(img->surf->pixels);
    Uint32* dst;
    
    
    int pitch = img->surf->pitch;
    
    // Compute the mandlebrot set.
    double MBSetf[Width*Height]; // 11 bits of exponent and 52 of fraction (1 of sign) 64 bits
    Mandel(MBSetf,xp,yp,zoom);
    Uint8 MBSeti[Width*Height];
    // Map the sets values onto integers.
    double mx = 0.0;
    for (int i=0; i<Width*Height; ++i)
    {
        if (MBSetf[i]>mx)
            mx = MBSetf[i];
    }
    for (int i=0; i<Width*Height; ++i)
        MBSeti[i] = (Uint8)(UINT8_MAX*(MBSetf[i]/mx));
    
    
    for (int row = 0; row < Height; ++row)
    {
        dst = (Uint32*)((Uint8*)pixels + row * pitch);
        for (int col = 0; col < Width; ++col)
        {
            // cout << MBSeti[row*Width+col]  << " " ;
            //bitmask_t color[3];
            //hilbert_i2c( 3, 8, ((MBSeti[row*Width+col]+loops)) , color);
            // Alpha RGB. Alpha must be FF to blit ...
            *dst++ = (  ((Uint32)MBSeti[row*Width+col] << 16) | ((Uint32)0xFF000000)); // | ((Uint32)color[1] << 8)
            //*dst++ = (  ((Uint32)color[0] << 16) | ((Uint32)color[1] << 8) | ((Uint32)color[2]) | ((Uint32)0xFF000000)); // | ((Uint32)color[1] << 8)
            
        }
    }
    
    
    SDL_UnlockSurface(img->surf);
    return img;
}

//
// GApp - A Graphical application that can use graphics, fonts, sounds, etc...
//
class GApp
{
public:
    typedef std::vector<GObj*> oblst_t;
    oblst_t objs; // Objects to be rendered.
    
    // Initalizes all the required SDL crap.
    GApp()
    {
        if (TTF_Init() != 0){
            cout << "Font init failure... " << endl;
            throw;
        }
        
        const std::string FontPath = getPath()+"sample.ttf";
        cout << "Try font open: " << FontPath.c_str() << endl;
        font = TTF_OpenFont(FontPath.c_str(), 64);
        if (font == nullptr){
            cout << "Font open Failure..." <<  TTF_GetError()<<  endl;
            throw;
        }
        
        /* Enable standard application logging */
        SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);
        
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s\n", SDL_GetError());
            throw;
        }
        
        /* Create the window and renderer */
        window = SDL_CreateWindow("Mandelbrot Explorer",
                                  SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED,
                                  Width, Height,
                                  SDL_WINDOW_SHOWN);
        if (!window) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't set create window: %s\n", SDL_GetError());
            throw;
        }
        
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't set create renderer: %s\n", SDL_GetError());
            throw;
        }
        
        // The screen's texture.
        /*
         texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, Width, Height);
         if (!texture) {
         SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't set create texture: %s\n", SDL_GetError());
         throw;
         }
         */
    }
    
    // Blit together the surfaces of Obj. Render them to a texture and to screen.
    void Render()
    {
        SDL_RenderClear(renderer);
        SDL_Surface* screen = SDL_CreateRGBSurface(0,Width,Height,32,0,0,0,0);
        
        SDL_SetSurfaceBlendMode(screen, SDL_BLENDMODE_ADD);
        
        
        if (!screen)
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "No Screen surface: %s\n", SDL_GetError());
        }
        
        // Blitting loop.
        for (oblst_t::iterator it = objs.begin(); it != objs.end(); ++it)
        {
            if (!(*it)->surf)
            {
                cout << "can't blit onto blank." << endl;
                throw;
            }
            cout << "Blitting:" << (*it)->surf->w << " x " << (*it)->surf->h << endl;
            cout << "Onto:" << screen->w << " x " << screen->h << endl;
            SDL_Rect Dest;
            Dest.x=0; Dest.y=0;
            Dest.w=Width; Dest.h=Height;
            SDL_BlitSurface((*it)->surf, NULL, screen, &Dest); // blit it to the screen
        }
        
        // Finally render them.
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, screen);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        
        SDL_DestroyTexture(texture);
        SDL_FreeSurface(screen);
    }
    
    // For now just make a text surface.
    void FormSurfaces()
    {
        // Delete all the objects.
        while (objs.size() > 0)
        {
            delete objs.back();
            objs.pop_back();
        }
        
        if (objs.size() == 0)
            objs.push_back(LoopText());
        
        // Also make a mandlebrot surface.
        objs.push_back(MandelImg());
        
        cout << "Have: " << objs.size() << " Surfaces " << endl;
        
        /*
         cout << "Surf" << surf->w << " " << surf->h << " " << surf->pitch << endl;
         SDL_Texture *text_text = SDL_CreateTextureFromSurface(renderer, surf);
         SDL_FreeSurface(surf);
         int iW, iH;
         SDL_QueryTexture(text_text, NULL, NULL, &iW, &iH);
         cout << "x:" << iW << " y:" << iH << endl;
         int x = Width / 2 - iW / 2;
         int y = Height / 2 - iH / 2;
         renderTextures(texture, texture, renderer, x, y);
         */
    }
    
    void Loop()
    {
        while(loops<40000 && !done)
        {
            cout << " loops " << loops << endl;
            // Recieve input.
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                switch (event.type)
                {
                    case SDL_KEYDOWN:
                        if (event.key.keysym.sym == SDLK_ESCAPE)
                        {
                            done = SDL_TRUE;
                            break;
                        }
                        if (event.key.keysym.sym == SDLK_i)
                        {
                            cout << "zoom in: "<< zoom << endl;
                            zoom *=1.2;   break; }
                        if (event.key.keysym.sym == SDLK_o)
                        {    zoom *=0.8;   break; }

                        if (event.key.keysym.sym == SDLK_w)
                        {    yp += 0.1;   break; }
                        if (event.key.keysym.sym == SDLK_s)
                        {    yp -= 0.1;   break; }
                        
                        if (event.key.keysym.sym == SDLK_a)
                        {    xp -= 0.1;   break; }
                        if (event.key.keysym.sym == SDLK_d)
                        {    xp += 0.1;   break; }
                        
                        break;
                    case SDL_QUIT:
                        done = SDL_TRUE;
                        break;
                }
            }
            
            FormSurfaces();
            // Update Objects.
            Render();
            loops++;
        }
    }
    
    ~GApp()
    {
        SDL_DestroyRenderer(renderer);
        TTF_CloseFont(font);
        SDL_Quit();
    }
    
};

#endif
