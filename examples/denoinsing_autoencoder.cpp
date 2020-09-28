/* "simplest", example of simply enumerating the available devices with ESCAPI */
#include <stdio.h>
#include "../src/escapi.h"
#include "../src/conv.hpp"
#include <limits>
#include <math.h>       /* log2 */
#include <xtensor/xadapt.hpp>
#include <xtensor/xrandom.hpp>
#define SDL_MAIN_HANDLED
#include <SDL.h>

const int int_bytes = std::ceil((std::log2(std::numeric_limits<int>::max()))/8);


void main()
{
	bool quit = false;
    SDL_Event event;
    SDL_Init(SDL_INIT_VIDEO);

	SDL_Window * cam_window = SDL_CreateWindow("Camera Input",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 320, 240, 0);

	SDL_Window * noised_window = SDL_CreateWindow("Input with Noise Added",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 320, 240, 0);

	SDL_Window * out_window = SDL_CreateWindow("Convolved Output",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 320, 240, 0);

	SDL_Window * err_window = SDL_CreateWindow("Output Error",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 320, 240, 0);

	SDL_Renderer * cam_renderer = SDL_CreateRenderer(cam_window, -1, 0);
	SDL_Renderer * noised_renderer = SDL_CreateRenderer(noised_window, -1, 0);
	SDL_Renderer * out_renderer = SDL_CreateRenderer(out_window, -1, 0);
	SDL_Renderer * err_renderer = SDL_CreateRenderer(err_window, -1, 0);

	/* Initialize ESCAPI */
	
	int devices = setupESCAPI();

	if (devices == 0)
	{
		printf("ESCAPI initialization failure or no devices found.\n");
		return;
	}

	struct SimpleCapParams capture;
	capture.mWidth = 320;
	capture.mHeight = 240;
	capture.mTargetBuf = new int[320 * 240];

	xt::static_shape<std::size_t, 4> pic_shape = { 1, 320, 240, int_bytes};
	
	if (initCapture(0, &capture) == 0)
	{
		printf("Capture failed - device may already be in use.\n");
		return;
	}

	xt::xtensor<float, 4> filter = xt::random::rand<float>({3,3,3,3}, 0, 120);
	xt::xtensor<float, 4> d_filter = xt::zeros<float>({3,3,3,3});

	SDL_Texture * img_texture = SDL_CreateTexture(cam_renderer, SDL_PIXELFORMAT_ARGB32, SDL_TEXTUREACCESS_STREAMING, 320, 240);
	SDL_Texture * noised_texture = SDL_CreateTexture(noised_renderer, SDL_PIXELFORMAT_ARGB32, SDL_TEXTUREACCESS_STREAMING, 320, 240);
	SDL_Texture * out_texture = SDL_CreateTexture(out_renderer, SDL_PIXELFORMAT_ARGB32, SDL_TEXTUREACCESS_STREAMING, 320, 240);
	SDL_Texture * err_texture = SDL_CreateTexture(err_renderer, SDL_PIXELFORMAT_ARGB32, SDL_TEXTUREACCESS_STREAMING, 320, 240);

	while(!quit){
        doCapture(0);
		
		while (isCaptureDone(0) == 0)
		{}

		// Instant conversion motherfuckers!!! >=D
		uint8_t* pic = (uint8_t*)static_cast<void*>(capture.mTargetBuf);
		auto img = xt::adapt(pic, 320 * 240*int_bytes, false, pic_shape);

		SDL_UpdateTexture(img_texture, NULL, static_cast<void*>(img.data()), 320*int_bytes);
		SDL_RenderCopy(cam_renderer, img_texture, NULL, NULL);
		SDL_RenderPresent(cam_renderer);

		auto a = xt::transpose(img, {0,3,2,1}); // to tensor shape
		auto r = xt::random::randint<int>(a.shape(), 0, 120);

		xt::xtensor<uint8_t, 4> noised_img = a+r;
		auto noised_img_display = xt::transpose(img, {0,3,2,1}); // to image shape

		SDL_UpdateTexture(noised_texture, NULL, static_cast<void*>(noised_img_display.data()), 320*int_bytes);
		SDL_RenderCopy(noised_renderer, noised_texture, NULL, NULL);
		SDL_RenderPresent(noised_renderer);

		auto pada = pad(a);
		auto d_inp = xt::zeros<float>(a.shape());

        auto res = conv2d_forward(pada, filter);
		auto res_display = xt::transpose(img, {0,3,2,1}); // to image shape


		SDL_UpdateTexture(out_texture, NULL, static_cast<void*>(res_display.data()), 320*int_bytes);
		SDL_RenderCopy(out_renderer, out_texture, NULL, NULL);
		SDL_RenderPresent(out_renderer);

		xt::xtensor<float, 4> res_cast = res;
		xt::xtensor<float, 4> img_cast = img;
		auto err = res_cast - img_cast;

		/*auto err_display = xt::transpose(err, {0,3,2,1}); // to image shape

		SDL_UpdateTexture(err_texture, NULL, static_cast<void*>(err_display.data()), 320*int_bytes);
		SDL_RenderCopy(err_renderer, err_texture, NULL, NULL);
		SDL_RenderPresent(err_renderer);*/

		conv2d_backward(pada, d_inp, filter, d_filter, err);
		xt::view(filter, xt::all(), xt::all(), xt::all(), xt::all()) -= xt::view(d_filter, xt::all(), xt::all(), xt::all(), xt::all())/1.0e8;

		SDL_WaitEventTimeout(&event, 0);
 
        switch (event.type)
        {
        case SDL_QUIT:
            quit = true;
            break;
        }
    }
	
	SDL_Quit();
	deinitCapture(0);	
}