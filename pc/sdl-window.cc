#include "sdl-window.h"

#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>

#include "gui-parameter.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#if defined(SDL_VIDEO_DRIVER_X11)
#include "SDL_syswm.h"
#endif

#if defined(__cplusplus) && __cplusplus >= 201703L && defined(__has_include)
#if __has_include(<filesystem>)
#define GHC_USE_STD_FS
#include <filesystem>
namespace fs = std::filesystem;
#endif
#endif
#ifndef GHC_USE_STD_FS
#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#ifdef __APPLE__
#ifdef __clang__
// Suppress deprecated warning of using legacy OpenGL API
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
#endif

#if 0
static void ClickFunc(GLFWwindow *window, int button, int action, int mods) {
  (void)mods;

  auto *params =
      reinterpret_cast<GuiParameter *>(glfwGetWindowUserPointer(window));

  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    if (action == GLFW_PRESS) {
      params->mouse_left_pressed = true;
    } else if (action == GLFW_RELEASE) {
      params->mouse_left_pressed = false;
    }
  }
  if (button == GLFW_MOUSE_BUTTON_RIGHT) {
    if (action == GLFW_PRESS) {
      params->mouse_righ_tPressed = true;
    } else if (action == GLFW_RELEASE) {
      params->mouse_righ_tPressed = false;
    }
  }
  if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
    if (action == GLFW_PRESS) {
      params->mouse_midd_lePressed = true;
    } else if (action == GLFW_RELEASE) {
      params->mouse_midd_lePressed = false;
    }
  }
}

static void MotionFunc(GLFWwindow *window, double mouse_x, double mouse_y) {
  const float kTransScale = 2.0f;

  auto *params =
      reinterpret_cast<GuiParameter *>(glfwGetWindowUserPointer(window));

  if (!ImGui::GetIO().WantCaptureMouse) {
    if (params->mouse_left_pressed) {
      // TODO
    } else if (params->mouse_midd_lePressed) {
      params->eye[0] -= kTransScale * (float(mouse_x) - params->prev_mouse_x) /
                        float(params->width);
      params->lookat[0] -= kTransScale *
                           (float(mouse_x) - params->prev_mouse_x) /
                           float(params->width);
      params->eye[1] += kTransScale * (float(mouse_y) - params->prev_mouse_y) /
                        float(params->height);
      params->lookat[1] += kTransScale *
                           (float(mouse_y) - params->prev_mouse_y) /
                           float(params->height);
    } else if (params->mouse_righ_tPressed) {
      params->eye[2] += kTransScale * (float(mouse_y) - params->prev_mouse_y) /
                        float(params->height);
      params->lookat[2] += kTransScale *
                           (float(mouse_y) - params->prev_mouse_y) /
                           float(params->height);
    }
  }

  // Update mouse point
  params->prev_mouse_x = float(mouse_x);
  params->prev_mouse_y = float(mouse_y);
}
#endif

inline uint8_t ftouc(float f) {
  int val = int(f * 255.0f);
  val     = (std::max)(0, (std::min)(255, val));

  return static_cast<uint8_t>(val);
}

// -------------------------------------------------------------
// https://www.nayuki.io/page/srgb-transform-library
//
/*
 * sRGB transform (C++)
 *
 * Copyright (c) 2017 Project Nayuki. (MIT License)
 * https://www.nayuki.io/page/srgb-transform-library
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * - The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 * - The Software is provided "as is", without warranty of any kind, express or
 *   implied, including but not limited to the warranties of merchantability,
 *   fitness for a particular purpose and noninfringement. In no event shall the
 *   authors or copyright holders be liable for any claim, damages or other
 *   liability, whether in an action of contract, tort or otherwise, arising
 * from, out of or in connection with the Software or the use or other dealings
 * in the Software.
 */

// TODO: Use pow table for faster conversion.
inline float linearToSrgb(float x) {
  if (x <= 0.0f)
    return 0.0f;
  else if (x >= 1.0f)
    return 1.0f;
  else if (x < 0.0031308f)
    return x * 12.92f;
  else
    return std::pow(x, 1.0f / 2.4f) * 1.055f - 0.055f;
}

const float SRGB_8BIT_TO_LINEAR_FLOAT[1 << 8] = {
    0.0f,          3.03527e-4f,   6.07054e-4f,   9.10581e-4f,   0.001214108f,
    0.001517635f,  0.001821162f,  0.0021246888f, 0.002428216f,  0.002731743f,
    0.00303527f,   0.0033465358f, 0.0036765074f, 0.004024717f,  0.004391442f,
    0.0047769537f, 0.005181517f,  0.005605392f,  0.0060488335f, 0.006512091f,
    0.0069954107f, 0.007499032f,  0.008023193f,  0.008568126f,  0.009134059f,
    0.009721218f,  0.010329823f,  0.010960095f,  0.011612245f,  0.012286489f,
    0.0129830325f, 0.013702083f,  0.014443845f,  0.015208516f,  0.015996294f,
    0.016807377f,  0.017641956f,  0.018500222f,  0.019382363f,  0.020288564f,
    0.021219011f,  0.022173885f,  0.023153368f,  0.024157634f,  0.025186861f,
    0.026241222f,  0.027320893f,  0.02842604f,   0.029556835f,  0.030713445f,
    0.031896032f,  0.033104766f,  0.034339808f,  0.035601314f,  0.036889452f,
    0.038204372f,  0.039546236f,  0.0409152f,    0.04231141f,   0.04373503f,
    0.045186203f,  0.046665087f,  0.048171826f,  0.049706567f,  0.051269464f,
    0.05286065f,   0.05448028f,   0.056128494f,  0.057805438f,  0.059511244f,
    0.06124606f,   0.06301002f,   0.06480327f,   0.066625945f,  0.068478175f,
    0.0703601f,    0.07227185f,   0.07421357f,   0.07618539f,   0.07818743f,
    0.08021983f,   0.082282715f,  0.084376216f,  0.086500466f,  0.08865559f,
    0.09084172f,   0.093058966f,  0.09530747f,   0.097587354f,  0.09989873f,
    0.10224174f,   0.10461649f,   0.107023105f,  0.10946172f,   0.111932434f,
    0.11443538f,   0.11697067f,   0.119538434f,  0.122138776f,  0.12477182f,
    0.12743768f,   0.13013647f,   0.13286832f,   0.13563333f,   0.13843162f,
    0.14126329f,   0.14412847f,   0.14702727f,   0.14995979f,   0.15292616f,
    0.15592647f,   0.15896083f,   0.16202939f,   0.1651322f,    0.1682694f,
    0.17144111f,   0.1746474f,    0.17788842f,   0.18116425f,   0.18447499f,
    0.18782078f,   0.19120169f,   0.19461784f,   0.19806932f,   0.20155625f,
    0.20507874f,   0.20863687f,   0.21223076f,   0.21586053f,   0.21952623f,
    0.22322798f,   0.2269659f,    0.23074007f,   0.23455061f,   0.2383976f,
    0.24228115f,   0.24620135f,   0.2501583f,    0.25415212f,   0.25818288f,
    0.2622507f,    0.26635563f,   0.27049783f,   0.27467734f,   0.2788943f,
    0.28314877f,   0.28744087f,   0.29177067f,   0.2961383f,    0.3005438f,
    0.30498734f,   0.30946895f,   0.31398875f,   0.3185468f,    0.32314324f,
    0.32777813f,   0.33245155f,   0.33716366f,   0.34191445f,   0.3467041f,
    0.35153264f,   0.35640016f,   0.36130682f,   0.36625263f,   0.3712377f,
    0.37626216f,   0.38132605f,   0.38642946f,   0.3915725f,    0.39675525f,
    0.4019778f,    0.40724024f,   0.41254264f,   0.4178851f,    0.4232677f,
    0.42869052f,   0.43415368f,   0.4396572f,    0.44520122f,   0.45078582f,
    0.45641103f,   0.46207702f,   0.4677838f,    0.4735315f,    0.4793202f,
    0.48514995f,   0.4910209f,    0.496933f,     0.5028865f,    0.50888133f,
    0.5149177f,    0.5209956f,    0.52711517f,   0.53327644f,   0.5394795f,
    0.5457245f,    0.55201143f,   0.55834043f,   0.5647115f,    0.57112485f,
    0.57758045f,   0.58407843f,   0.59061885f,   0.5972018f,    0.60382736f,
    0.61049557f,   0.6172066f,    0.62396044f,   0.63075715f,   0.6375969f,
    0.6444797f,    0.65140563f,   0.65837485f,   0.66538733f,   0.67244315f,
    0.6795425f,    0.6866853f,    0.6938718f,    0.7011019f,    0.7083758f,
    0.71569353f,   0.7230551f,    0.73046076f,   0.73791045f,   0.74540424f,
    0.7529422f,    0.7605245f,    0.76815116f,   0.7758222f,    0.7835378f,
    0.791298f,     0.7991027f,    0.8069523f,    0.8148466f,    0.82278574f,
    0.8307699f,    0.838799f,     0.8468732f,    0.8549926f,    0.8631572f,
    0.8713671f,    0.8796224f,    0.8879231f,    0.8962694f,    0.9046612f,
    0.91309863f,   0.92158186f,   0.9301109f,    0.9386857f,    0.9473065f,
    0.9559733f,    0.9646863f,    0.9734453f,    0.9822506f,    0.9911021f,
    1.0f,
};

const double SRGB_8BIT_TO_LINEAR_DOUBLE[1 << 8] = {
    0.0,
    3.035269835488375e-4,
    6.07053967097675e-4,
    9.105809506465125e-4,
    0.00121410793419535,
    0.0015176349177441874,
    0.001821161901293025,
    0.0021246888848418626,
    0.0024282158683907,
    0.0027317428519395373,
    0.003035269835488375,
    0.003346535763899161,
    0.003676507324047436,
    0.004024717018496307,
    0.004391442037410293,
    0.004776953480693729,
    0.005181516702338386,
    0.005605391624202723,
    0.006048833022857054,
    0.006512090792594475,
    0.006995410187265387,
    0.007499032043226175,
    0.008023192985384994,
    0.008568125618069307,
    0.009134058702220787,
    0.00972121732023785,
    0.010329823029626936,
    0.010960094006488246,
    0.011612245179743885,
    0.012286488356915872,
    0.012983032342173012,
    0.013702083047289686,
    0.014443843596092545,
    0.01520851442291271,
    0.01599629336550963,
    0.016807375752887384,
    0.017641954488384078,
    0.018500220128379697,
    0.019382360956935723,
    0.0202885630566524,
    0.021219010376003555,
    0.022173884793387385,
    0.02315336617811041,
    0.024157632448504756,
    0.02518685962736163,
    0.026241221894849898,
    0.027320891639074894,
    0.028426039504420793,
    0.0295568344378088,
    0.030713443732993635,
    0.03189603307301153,
    0.033104766570885055,
    0.03433980680868217,
    0.03560131487502034,
    0.03688945040110004,
    0.0382043715953465,
    0.03954623527673284,
    0.04091519690685319,
    0.042311410620809675,
    0.043735029256973465,
    0.04518620438567554,
    0.046665086336880095,
    0.04817182422688942,
    0.04970656598412723,
    0.05126945837404324,
    0.052860647023180246,
    0.05448027644244237,
    0.05612849004960009,
    0.05780543019106723,
    0.0595112381629812,
    0.06124605423161761,
    0.06301001765316767,
    0.06480326669290577,
    0.06662593864377289,
    0.06847816984440017,
    0.07036009569659588,
    0.07227185068231748,
    0.07421356838014963,
    0.07618538148130785,
    0.07818742180518633,
    0.08021982031446832,
    0.0822827071298148,
    0.08437621154414882,
    0.08650046203654976,
    0.08865558628577294,
    0.09084171118340768,
    0.09305896284668745,
    0.0953074666309647,
    0.09758734714186246,
    0.09989872824711389,
    0.10224173308810132,
    0.10461648409110419,
    0.10702310297826761,
    0.10946171077829933,
    0.1119324278369056,
    0.11443537382697373,
    0.11697066775851084,
    0.11953842798834562,
    0.12213877222960187,
    0.12477181756095049,
    0.12743768043564743,
    0.1301364766903643,
    0.13286832155381798,
    0.13563332965520566,
    0.13843161503245183,
    0.14126329114027164,
    0.14412847085805777,
    0.14702726649759498,
    0.14995978981060856,
    0.15292615199615017,
    0.1559264637078274,
    0.1589608350608804,
    0.162029375639111,
    0.1651321945016676,
    0.16826940018969075,
    0.1714411007328226,
    0.17464740365558504,
    0.17788841598362912,
    0.18116424424986022,
    0.184474994500441,
    0.18782077230067787,
    0.19120168274079138,
    0.1946178304415758,
    0.19806931955994886,
    0.20155625379439707,
    0.20507873639031693,
    0.20863687014525575,
    0.21223075741405523,
    0.21586050011389926,
    0.2195261997292692,
    0.2232279573168085,
    0.22696587351009836,
    0.23074004852434915,
    0.23455058216100522,
    0.238397573812271,
    0.24228112246555486,
    0.24620132670783548,
    0.25015828472995344,
    0.25415209433082675,
    0.2581828529215958,
    0.26225065752969623,
    0.26635560480286247,
    0.2704977910130658,
    0.27467731206038465,
    0.2788942634768104,
    0.2831487404299921,
    0.2874408377269175,
    0.29177064981753587,
    0.2961382707983211,
    0.3005437944157765,
    0.3049873140698863,
    0.30946892281750854,
    0.31398871337571754,
    0.31854677812509186,
    0.32314320911295075,
    0.3277780980565422,
    0.33245153634617935,
    0.33716361504833037,
    0.3419144249086609,
    0.3467040563550296,
    0.35153259950043936,
    0.3564001441459435,
    0.3613067797835095,
    0.3662525955988395,
    0.3712376804741491,
    0.3762621229909065,
    0.38132601143253014,
    0.386429433787049,
    0.39157247774972326,
    0.39675523072562685,
    0.4019777798321958,
    0.4072402119017367,
    0.41254261348390375,
    0.4178850708481375,
    0.4232676699860717,
    0.4286904966139066,
    0.43415363617474895,
    0.4396571738409188,
    0.44520119451622786,
    0.45078578283822346,
    0.45641102318040466,
    0.4620769996544071,
    0.467783796112159,
    0.47353149614800955,
    0.4793201831008268,
    0.4851499400560704,
    0.4910208498478356,
    0.4969329950608704,
    0.5028864580325687,
    0.5088813208549338,
    0.5149176653765214,
    0.5209955732043543,
    0.5271151257058131,
    0.5332764040105052,
    0.5394794890121072,
    0.5457244613701866,
    0.5520114015120001,
    0.5583403896342679,
    0.5647115057049292,
    0.5711248294648731,
    0.5775804404296506,
    0.5840784178911641,
    0.5906188409193369,
    0.5972017883637634,
    0.6038273388553378,
    0.6104955708078648,
    0.6172065624196511,
    0.6239603916750761,
    0.6307571363461468,
    0.6375968739940326,
    0.6444796819705821,
    0.6514056374198242,
    0.6583748172794485,
    0.665387298282272,
    0.6724431569576875,
    0.6795424696330938,
    0.6866853124353135,
    0.6938717612919899,
    0.7011018919329731,
    0.7083757798916868,
    0.7156935005064807,
    0.7230551289219693,
    0.7304607400903537,
    0.7379104087727308,
    0.7454042095403874,
    0.7529422167760779,
    0.7605245046752924,
    0.768151147247507,
    0.7758222183174236,
    0.7835377915261935,
    0.7912979403326302,
    0.799102738014409,
    0.8069522576692516,
    0.8148465722161012,
    0.8227857543962835,
    0.8307698767746546,
    0.83879901174074,
    0.846873231509858,
    0.8549926081242338,
    0.8631572134541023,
    0.8713671191987972,
    0.8796223968878317,
    0.8879231178819663,
    0.8962693533742664,
    0.9046611743911496,
    0.9130986517934192,
    0.9215818562772946,
    0.9301108583754237,
    0.938685728457888,
    0.9473065367331999,
    0.9559733532492861,
    0.9646862478944651,
    0.9734452903984125,
    0.9822505503331171,
    0.9911020971138298,
    1.0,
};

uint8_t linearToSrgb8bit(float x) {
  if (x <= 0.0f) return 0;
  if (x >= 1.0f) return 255;
  const float *TABLE = SRGB_8BIT_TO_LINEAR_FLOAT;
  int y              = 0;
  for (int i = 128; i != 0; i >>= 1) {
    if (TABLE[y + i] <= x) y += i;
  }
  if (x - TABLE[y] <= TABLE[y + 1] - x)
    return static_cast<uint8_t>((std::max)(0, (std::min)(255, y)));
  else
    return static_cast<uint8_t>((std::max)(0, (std::min)(255, y + 1)));
}

uint8_t linearToSrgb8bit(double x) {
  if (x <= 0.0) return 0;
  if (x >= 1.0) return 255;
  const double *TABLE = SRGB_8BIT_TO_LINEAR_DOUBLE;
  int y               = 0;
  for (int i = 128; i != 0; i >>= 1) {
    if (TABLE[y + i] <= x) y += i;
  }
  if (x - TABLE[y] <= TABLE[y + 1] - x)
    return static_cast<uint8_t>((std::max)(0, (std::min)(255, y)));
  else
    return static_cast<uint8_t>((std::max)(0, (std::min)(255, y + 1)));
}

// -------------------------------------------------------------

void UpdateTexutre(SDL_Texture *tex, const ImageBuffer *imgbuf) {
  if (!imgbuf) {
    std::cerr << "imgbuf is nullptr.\n";
    // ???
    return;
  }

  int w, h;
  SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);

  if ((imgbuf->width != w) || (imgbuf->height != h)) {
    std::cerr << "texture size and ImageBuffer size mismatch\n";
    return;
  }

  std::vector<uint8_t> buf;
  buf.resize(imgbuf->width * imgbuf->height * 4);

  for (size_t i = 0; i < imgbuf->width * imgbuf->height; i++) {
#if 0 
      //uint8_t r = ftouc(linearToSrgb(aov.rgb[3 * i + 0]));
      //uint8_t g = ftouc(linearToSrgb(aov.rgb[3 * i + 1]));
      //uint8_t b = ftouc(linearToSrgb(aov.rgb[3 * i + 2]));
#else  // faster code
    uint8_t r = linearToSrgb8bit(imgbuf->buffer[4 * i + 0]);
    uint8_t g = linearToSrgb8bit(imgbuf->buffer[4 * i + 1]);
    uint8_t b = linearToSrgb8bit(imgbuf->buffer[4 * i + 2]);
#endif

    buf[4 * i + 0] = r;
    buf[4 * i + 1] = g;
    buf[4 * i + 2] = b;
    buf[4 * i + 3] = 255;
  }

  SDL_UpdateTexture(tex, nullptr, reinterpret_cast<const void *>(buf.data()),
                    imgbuf->width * 4);
}

static void InitializeImgui(SDL_Window *window, SDL_Renderer *renderer) {
  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();

  ImGui::CreateContext();
  auto &io = ImGui::GetIO();

  // Read .ini file from parent directory if imgui.ini does not exist in the
  // current directory.
  if (fs::exists("../imgui.ini") && !fs::exists("./imgui.ini")) {
    printf("Use ../imgui.ini as Init file.\n");
    io.IniFilename = "../imgui.ini";
  }

  // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  //  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

  float default_font_scale = 18.0f;
  ImFontConfig roboto_config;
  strcpy(roboto_config.Name, "Roboto");
  roboto_config.SizePixels = default_font_scale;

  io.Fonts->AddFontFromMemoryCompressedTTF(roboto_mono_compressed_data,
                                           roboto_mono_compressed_size,
                                           default_font_scale, &roboto_config);

  // Load Icon fonts
  ImFontConfig ionicons_config;
  ionicons_config.MergeMode          = true;
  ionicons_config.GlyphMinAdvanceX   = default_font_scale;
  ionicons_config.OversampleH        = 1;
  ionicons_config.OversampleV        = 1;
  static const ImWchar icon_ranges[] = {ICON_MIN_II, ICON_MAX_II, 0};
  io.Fonts->AddFontFromMemoryCompressedTTF(
      ionicons_compressed_data, ionicons_compressed_size, default_font_scale,
      &ionicons_config, icon_ranges);

  ImGui::StyleColorsDark();

  // Setup Platform/Renderer bindings
  ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer_Init(renderer);
  std::cout << "Init SDL2 with SDLRenderer\n";
}

static void ScreenActivate(SDL_Window *window) {
#if defined(SDL_VIDEO_DRIVER_X11)
  SDL_SysWMinfo wm;

  // Get window info.
  SDL_VERSION(&wm.version);
  SDL_GetWindowWMInfo(window, &wm);

  // Lock to display access.
  // wm.info.x11.lock_func();

  // Show the window on top.
  XMapRaised(wm.info.x11.display, wm.info.x11.window);

  // Set the focus on it.
  XSetInputFocus(wm.info.x11.display, wm.info.x11.window, RevertToParent,
                 CurrentTime);
#else
  (void)window;
#endif
}

SDLWindow::SDLWindow(int win_width, int win_height, const char *title) {
#if defined(__APPLE__)
  // For some reason, HIGHDPI does not work well on Retina Display for
  // SDLRenderer backend. Disable it for a while.
  SDL_WindowFlags window_flags =
      static_cast<SDL_WindowFlags>(SDL_WINDOW_RESIZABLE);
#else
  SDL_WindowFlags window_flags = static_cast<SDL_WindowFlags>(
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
#endif

  window_ =
      SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                       win_width, win_height, window_flags);
  if (!window_) {
    std::cerr << "Failed to create SDL2 window. If you are running on Linux, "
                 "probably X11 Display is not setup correctly. Check your "
                 "DISPLAY environment.\n";
    exit(-1);
  }

  std::cout << "SDL2 Window creation OK\n";

  renderer_ = SDL_CreateRenderer(
      window_, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

  if (!renderer_) {
    std::cerr << "Failed to create SDL2 renderer. If you are running on "
                 "Linux, "
                 "probably X11 Display is not setup correctly. Check your "
                 "DISPLAY environment.\n";
    exit(-1);
  }

  std::cout << "SDL2 Renderer creation OK\n";

#if 0
  // register gui_param pointer in this instance
  glfwSetWindowUserPointer(window_, &gui_param_);

  // Register Callback funtion that is called when window size is changed
  glfwSetWindowSizeCallback(window_, ReshapeFunc);

  // Register Callback funtion that is called when keyboard is inputed
  glfwSetKeyCallback(window_, KeyboardFunc);

  // Register Callback funtion that is called when mouse button is pushed
  glfwSetMouseButtonCallback(window_, ClickFunc);

  // Register Callback funtion that is called when mouse is moved
  glfwSetCursorPosCallback(window_, MotionFunc);

  // make this window target
  glfwMakeContextCurrent(window_);
#endif

  InitializeImgui(window_, renderer_);

  std::cout << "Compiling GL shaders\n";

  // FIXME
  int width = 1024;
  int height = 1024;

  texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32,
                               SDL_TEXTUREACCESS_TARGET, width, height);

  SDL_SetRenderTarget(renderer_, texture_);
  SDL_SetRenderDrawColor(renderer_, 255, 0, 255, 255);
  SDL_RenderClear(renderer_);
  SDL_SetRenderTarget(renderer_, nullptr);

  std::cout << "DONE SDLWindow creation\n";
}

SDLWindow::~SDLWindow() {

  SDL_DestroyTexture(texture_);
  SDL_DestroyRenderer(renderer_);
  SDL_DestroyWindow(window_);

  SDL_Quit();

  // Cleanup
  ImGui_ImplSDLRenderer_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
}

size_t SDLWindow::CreateBuffer(const size_t width, const size_t height,
                               const size_t channel) {
  const size_t buffer_id = gui_param_.pImageBuffers.size();

  gui_param_.pImageBuffers.emplace_back(new ImageBuffer);
  auto &bf = gui_param_.pImageBuffers.back();

  {
    std::lock_guard<std::mutex> lock(bf->mtx);
    bf->buffer.resize(width * height * channel, 0.f);
    bf->width      = width;
    bf->height     = height;
    bf->channel    = channel;
    bf->has_update = true;
  }

  return buffer_id;
}

bool SDLWindow::SetCurrentBuffer(const size_t buffer_id) {
  if (buffer_id < gui_param_.pImageBuffers.size()) {
    gui_param_.current_buffer_id = buffer_id;
    return true;
  }
  return false;
}

void SDLWindow::SetRenderItem(std::shared_ptr<RenderItem> &pRenderItem) {
  gui_param_.pRenderItem = pRenderItem;
}

std::shared_ptr<ImageBuffer> SDLWindow::FetchBuffer(const size_t buffer_id) {
  return gui_param_.pImageBuffers.at(buffer_id);
}

void SDLWindow::DrawCurrentBuffer(void) {
  if (gui_param_.current_buffer_id >= gui_param_.pImageBuffers.size()) {
    return;
  }
  ImageBuffer *image_buffer =
      gui_param_.pImageBuffers[gui_param_.current_buffer_id].get();

  // Update texture
  {
    std::lock_guard<std::mutex> lock(image_buffer->mtx);
    if (image_buffer->has_update) {
      UpdateTexutre(texture_, image_buffer);

      image_buffer->has_update = false;
    }
  }

  // Rendered image is shown as Imgui Image.
  ImGui::Begin("Image");
  ImGui::Image(texture_, ImVec2(image_buffer->width, image_buffer->height));
  ImGui::End();

  SDL_SetRenderDrawColor(renderer_, 114, 144, 154, 255);
  SDL_RenderClear(renderer_);

}

static void RequestRerender(GuiParameter *gui_param) {
  gui_param->pRenderItem->cancel_render_flag.store(true);
  gui_param->pRenderItem->last_render_pass.store(0);
  gui_param->pRenderItem->finish_pass.store(0);
}

static void MainUI(GuiParameter *gui_param, bool *rerender) {
  ImGui::Begin("Render");

  const float progress = float(gui_param->pRenderItem->finish_pass.load()) /
                         float(gui_param->pRenderItem->max_pass.load());
  std::stringstream ss;
  ss << "Pass " << gui_param->pRenderItem->finish_pass << " of "
     << gui_param->pRenderItem->max_pass << " (" << progress * 100.f << "%)";
  ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f), ss.str().c_str());

  int _max_pass = int(gui_param->pRenderItem->max_pass);
  if (ImGui::DragInt("max_pass", &_max_pass, 1.f, 1,
                     (std::numeric_limits<int>::max)())) {
    gui_param->pRenderItem->max_pass = size_t(_max_pass);
    *rerender                        = true;
  }

  if (ImGui::Button("Rerender")) {
    *rerender = true;
  }

  ImGui::End();
}

static void ColorPicker3(const char *label, float *v, bool *update_material) {
  if (ImGui::TreeNodeEx("Color Picker", ImGuiTreeNodeFlags_NoAutoOpenOnLog)) {
    if (ImGui::ColorPicker3(label, v)) {
      *update_material = true;
    }
    ImGui::TreePop();
  }
}

static void CyclesPrincipledBsdfUI(
    pbrlab::CyclesPrincipledBsdfParameter *material_param,
    bool *update_material) {
  if (ImGui::TreeNodeEx("Base", ImGuiTreeNodeFlags_DefaultOpen)) {
    if (ImGui::DragFloat3("base color", material_param->base_color.v, 0.01f,
                          0.0f, 1.f)) {
      *update_material = true;
    }
    ColorPicker3("base color", material_param->base_color.v, update_material);

    if (material_param->base_color_tex_id != uint32_t(-1)) {
      ImGui::Text("use texture");
    }

    if (ImGui::DragFloat("metallic", &(material_param->metallic), 0.01f, 0.f,
                         1.f)) {
      *update_material = true;
    }
    ImGui::TreePop();
  }
  if (ImGui::TreeNodeEx("Subusuface", ImGuiTreeNodeFlags_NoAutoOpenOnLog)) {
    if (ImGui::DragFloat("subsurface", &(material_param->subsurface), 0.01f,
                         0.0f, 1.f)) {
      *update_material = true;
    }
    if (ImGui::DragFloat3("subsurface radius",
                          material_param->subsurface_radius.v, 0.01f, 0.0f)) {
      *update_material = true;
    }
    if (ImGui::DragFloat3("subsurface color",
                          material_param->subsurface_color.v, 0.01f, 0.0f,
                          1.f)) {
      *update_material = true;
    }
    ColorPicker3("subsurface color", material_param->subsurface_color.v,
                 update_material);

    if (material_param->subsurface_color_tex_id != uint32_t(-1)) {
      ImGui::Text("use texture");
    }

    ImGui::TreePop();
  }
  if (ImGui::TreeNodeEx("Specular", ImGuiTreeNodeFlags_NoAutoOpenOnLog)) {
    if (ImGui::DragFloat("specular", &(material_param->specular), 0.01f, 0.0f,
                         1.f)) {
      *update_material = true;
    }
    if (ImGui::DragFloat("specular tint", &(material_param->specular_tint),
                         0.01f, 0.0f, 1.f)) {
      *update_material = true;
    }
    if (ImGui::DragFloat("roughness", &(material_param->roughness), 0.01f, 0.0f,
                         1.f)) {
      *update_material = true;
    }
    if (ImGui::DragFloat("ior", &(material_param->ior), 0.01f, 0.0f)) {
      *update_material = true;
    }
    if (ImGui::DragFloat("anisotropic", &(material_param->anisotropic), 0.01f,
                         0.0f, 1.f)) {
      *update_material = true;
    }
    if (ImGui::DragFloat("anisotropic rotation",
                         &(material_param->anisotropic_rotation), 0.01f, 0.0f,
                         1.f)) {
      *update_material = true;
    }
    ImGui::TreePop();
  }
  if (ImGui::TreeNodeEx("Sheen", ImGuiTreeNodeFlags_NoAutoOpenOnLog)) {
    if (ImGui::DragFloat("sheen", &(material_param->sheen), 0.01f, 0.0f, 1.f)) {
      *update_material = true;
    }
    if (ImGui::DragFloat("sheen tint", &(material_param->sheen_tint), 0.01f,
                         0.0f, 1.f)) {
      *update_material = true;
    }
    ImGui::TreePop();
  }
  if (ImGui::TreeNodeEx("Clearcoat", ImGuiTreeNodeFlags_NoAutoOpenOnLog)) {
    if (ImGui::DragFloat("clearcoat", &(material_param->clearcoat), 0.01f, 0.0f,
                         1.f)) {
      *update_material = true;
    }
    if (ImGui::DragFloat("clearcoat roughness",
                         &(material_param->clearcoat_roughness), 0.01f, 0.0f,
                         1.f)) {
      *update_material = true;
    }
    ImGui::TreePop();
  }
  if (ImGui::TreeNodeEx("Transmission", ImGuiTreeNodeFlags_NoAutoOpenOnLog)) {
    if (ImGui::DragFloat("transmission", &(material_param->transmission), 0.01f,
                         0.0f, 1.f)) {
      *update_material = true;
    }
    if (ImGui::DragFloat("transmission roughness",
                         &(material_param->transmission_roughness), 0.01f, 0.0f,
                         1.f)) {
      *update_material = true;
    }
    ImGui::TreePop();
  }
}
static void HairBsdfUI(ImGuiParameter *imgui_parameter,
                       pbrlab::HairBsdfParameter *material_param,
                       bool *update_material) {
  if (ImGui::TreeNodeEx("Hair Color", ImGuiTreeNodeFlags_DefaultOpen)) {
    auto &current_hair_coloring = imgui_parameter->current_hair_coloring;
    const std::vector<std::string> &hair_coloring_names =
        imgui_parameter->hair_coloring_names;

    pbrlab::HairBsdfParameter::ColoringHair hair_coloring_type =
        pbrlab::HairBsdfParameter::ColoringHair(material_param->coloring_hair);

    current_hair_coloring =
        hair_coloring_names.at(size_t(hair_coloring_type)).c_str();

    if (ImGui::BeginCombo("Hair Coloring", current_hair_coloring)) {
      for (size_t n = 0; n < hair_coloring_names.size(); n++) {
        bool is_selected =
            (current_hair_coloring == hair_coloring_names[n].c_str());

        if (ImGui::Selectable(hair_coloring_names[n].c_str(), is_selected)) {
          current_hair_coloring = hair_coloring_names[n].c_str();
          hair_coloring_type    = pbrlab::HairBsdfParameter::ColoringHair(n);
          material_param->coloring_hair = hair_coloring_type;
          *update_material              = true;
        }
        if (is_selected) {
          ImGui::SetItemDefaultFocus();  // You may set the initial focus when
        }
      }
      ImGui::EndCombo();
    }

    if (hair_coloring_type == pbrlab::HairBsdfParameter::kRGB) {
      if (ImGui::DragFloat3("base color", material_param->base_color.v, 0.01f,
                            0.0f, 1.f)) {
        *update_material = true;
      }
      ColorPicker3("base color", material_param->base_color.v, update_material);
    } else if (hair_coloring_type == pbrlab::HairBsdfParameter::kMelanin) {
      if (ImGui::DragFloat("melanin", &(material_param->melanin), 0.01f, 0.0f,
                           1.f)) {
        *update_material = true;
      }
      if (ImGui::DragFloat("melanin redness",
                           &(material_param->melanin_redness), 0.01f, 0.0f,
                           1.f)) {
        *update_material = true;
      }
      if (ImGui::DragFloat("melanin randomize",
                           &(material_param->melanin_randomize), 0.05f, 0.0f,
                           1.f)) {
        *update_material = true;
      }
    } else {
      assert(false);
    }

    ImGui::TreePop();
  }
  if (ImGui::TreeNodeEx("Other", ImGuiTreeNodeFlags_DefaultOpen)) {
    if (ImGui::DragFloat("roughness", &(material_param->roughness), 0.01f, 0.0f,
                         1.f)) {
      *update_material = true;
    }
    if (ImGui::DragFloat("anisotropic roughness",
                         &(material_param->azimuthal_roughness), 0.01f, 0.0f,
                         1.f)) {
      *update_material = true;
    }
    if (ImGui::DragFloat("ior", &(material_param->ior), 0.01f, 0.0f, 1.f)) {
      *update_material = true;
    }
    if (ImGui::DragFloat("shift", &(material_param->shift), 0.01f, 0.0f, 1.f)) {
      *update_material = true;
    }
  }
  if (ImGui::TreeNodeEx("Tint", ImGuiTreeNodeFlags_DefaultOpen)) {
    if (ImGui::DragFloat3("specular tint", material_param->specular_tint.v,
                          0.01f, 0.0f, 1.f)) {
      *update_material = true;
    }
    ColorPicker3("specular tint", material_param->specular_tint.v,
                 update_material);
    if (ImGui::DragFloat3("second_specular tint",
                          material_param->second_specular_tint.v, 0.01f, 0.0f,
                          1.f)) {
      *update_material = true;
    }
    ColorPicker3("second_specular tint", material_param->second_specular_tint.v,
                 update_material);
    if (ImGui::DragFloat3("transmission tint",
                          material_param->transmission_tint.v, 0.01f, 0.0f,
                          1.f)) {
      *update_material = true;
    }

    ColorPicker3("transmission tint", material_param->transmission_tint.v,
                 update_material);
  }
}

static void MaterialUI(ImGuiParameter *imgui_parameter, pbrlab::Scene *scene,
                       EditQueue *edit_queue, bool *rerender) {
  bool update_material = false;
  std::vector<pbrlab::MaterialParameter> *materials =
      scene->FetchMeshMaterialParameters();
  ImGui::Begin("Material");

  if (materials->size() == 0) {
    ImGui::Text("No materials");
    ImGui::End();
    return;
  }
  if (ImGui::TreeNodeEx("Edit Material", ImGuiTreeNodeFlags_DefaultOpen)) {
    std::vector<std::string> &mtl_names = imgui_parameter->mtl_names;
    mtl_names.resize(materials->size());
    for (size_t i = 0; i < materials->size(); i++) {
      const std::string name = pbrlab::GetMaterialName((*materials)[i]);

      mtl_names[i] = name;
    }

    // Editing material selector
    size_t &mtl_idx = imgui_parameter->mtl_idx;

    auto &current_material = imgui_parameter->current_material;

    if (current_material == nullptr) {
      current_material = mtl_names.at(0).c_str();
    }

    if (ImGui::BeginCombo("material", current_material)) {
      for (size_t n = 0; n < mtl_names.size(); n++) {
        bool is_selected = (current_material == mtl_names[n].c_str());

        if (ImGui::Selectable(mtl_names[n].c_str(), is_selected)) {
          current_material = mtl_names[n].c_str();
          mtl_idx          = n;
        }
        if (is_selected) {
          ImGui::SetItemDefaultFocus();  // You may set the initial focus when
        }
      }
      ImGui::EndCombo();
    }

    if (materials->size() <= mtl_idx) {
      ImGui::End();
      return;
    }

    ImGui::Separator();

    pbrlab::MaterialParameter material = {};
    if (!edit_queue->FetchIfExit(materials->data() + mtl_idx, &material)) {
      material = materials->at(mtl_idx);
    }

    pbrlab::MaterialParameterType mtl_type =
        pbrlab::MaterialParameterType(material.index());

    const std::vector<std::string> &mtl_type_names =
        imgui_parameter->mtl_type_names;
    auto &current_material_type = imgui_parameter->current_material_type;

    current_material_type = mtl_type_names.at(size_t(mtl_type)).c_str();

    if (ImGui::BeginCombo("material type", current_material_type)) {
      for (size_t n = 0; n < mtl_type_names.size(); n++) {
        bool is_selected = (current_material_type == mtl_type_names[n].c_str());

        if (ImGui::Selectable(mtl_type_names[n].c_str(), is_selected)) {
          current_material_type = mtl_type_names[n].c_str();

          pbrlab::MaterialParameterType tmp_mtl_type =
              pbrlab::MaterialParameterType(n);
          if (tmp_mtl_type != mtl_type) {
            mtl_type        = tmp_mtl_type;
            update_material = true;
            if (mtl_type == pbrlab::kCyclesPrincipledBsdfParameter) {
              material = pbrlab::CyclesPrincipledBsdfParameter();
            } else if (mtl_type == pbrlab::kHairBsdfParameter) {
              material = pbrlab::HairBsdfParameter();
            } else {
              assert(false);
            }
          }
        }
        if (is_selected) {
          ImGui::SetItemDefaultFocus();  // You may set the initial focus when
        }
      }
      ImGui::EndCombo();
    }

    if (mtl_type == pbrlab::kCyclesPrincipledBsdfParameter) {
      pbrlab::CyclesPrincipledBsdfParameter &cycles_material =
          mpark::get<pbrlab::kCyclesPrincipledBsdfParameter>(material);
      CyclesPrincipledBsdfUI(&cycles_material, &update_material);
    } else if (mtl_type == pbrlab::kHairBsdfParameter) {
      pbrlab::HairBsdfParameter &hair_material =
          mpark::get<pbrlab::kHairBsdfParameter>(material);
      HairBsdfUI(imgui_parameter, &hair_material, &update_material);
    } else {
      assert(false);
    }

    if (update_material) {
      *rerender = true;
      edit_queue->Push(&material, &(materials->at(mtl_idx)));
    }
    ImGui::TreePop();
  }

  ImGui::End();
}

void SDLWindow::DrawImguiUI(void) {
  // start
  ImGui_ImplSDLRenderer_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  bool rerender = false;
  // draw some window
  MainUI(&gui_param_, &rerender);
  MaterialUI(&(gui_param_.imgui_parameter), &(gui_param_.pRenderItem->scene),
             &(gui_param_.pRenderItem->edit_queue), &rerender);

  // end
  ImGui::Render();

  SDL_RenderClear(renderer_);
  ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
  //SDL_RenderPresent(renderer_);

  if (rerender) {
    RequestRerender(&gui_param_);
  }
}

// Determine if the window should be closed
bool SDLWindow::ShouldClose() const {
  SDL_Event e;
  SDL_PollEvent(&e);
  if (e.type == SDL_QUIT) {
    return true;
  }

  return false;
}

// Fetch event with swapping color buffer
void SDLWindow::SwapBuffers() { SDL_RenderPresent(renderer_); }
