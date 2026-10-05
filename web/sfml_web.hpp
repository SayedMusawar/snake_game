// Minimal SFML 2.x look-alike built on SDL2 and SDL2_ttf.
// It implements only the SFML classes that main.cpp uses, and is compiled
// only for the browser build (Emscripten). The desktop build uses real SFML.
#pragma once

#include <SDL.h>
#include <SDL_ttf.h>

#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

namespace sf
{

    typedef std::uint8_t Uint8;

    template <typename T>
    struct Vector2
    {
        T x, y;
        Vector2() : x(0), y(0) {}
        Vector2(T X, T Y) : x(X), y(Y) {}
    };
    typedef Vector2<float> Vector2f;

    struct FloatRect
    {
        float left, top, width, height;
    };

    struct Color
    {
        Uint8 r, g, b, a;
        Color() : r(0), g(0), b(0), a(255) {}
        Color(Uint8 R, Uint8 G, Uint8 B, Uint8 A = 255) : r(R), g(G), b(B), a(A) {}
        static const Color White;
        static const Color Black;
    };
    inline const Color Color::White(255, 255, 255);
    inline const Color Color::Black(0, 0, 0);

    struct Vertex
    {
        Vector2f position;
        Color color;
        Vertex() {}
        Vertex(const Vector2f &p, const Color &c) : position(p), color(c) {}
    };

    enum PrimitiveType
    {
        Lines
    };

    class Time
    {
    public:
        explicit Time(float seconds = 0.f) : s_(seconds) {}
        float asSeconds() const { return s_; }

    private:
        float s_;
    };

    class Clock
    {
        typedef std::chrono::steady_clock C;

    public:
        Clock() : start_(C::now()) {}
        Time getElapsedTime() const { return Time(std::chrono::duration<float>(C::now() - start_).count()); }
        Time restart()
        {
            C::time_point n = C::now();
            Time t(std::chrono::duration<float>(n - start_).count());
            start_ = n;
            return t;
        }

    private:
        C::time_point start_;
    };

    struct Keyboard
    {
        enum Key
        {
            Unknown = -1,
            A,
            D,
            P,
            S,
            W,
            Left,
            Right,
            Up,
            Down,
            Enter
        };
    };

    struct Event
    {
        enum EventType
        {
            Closed,
            KeyPressed
        };
        struct KeyEvent
        {
            Keyboard::Key code;
        };
        EventType type;
        KeyEvent key;
        Event() : type(Closed) { key.code = Keyboard::Unknown; }
    };

    struct VideoMode
    {
        unsigned int width, height;
        VideoMode(unsigned int w, unsigned int h) : width(w), height(h) {}
    };

    class Font
    {
    public:
        Font() {}
        Font(const Font &) = delete;
        Font &operator=(const Font &) = delete;
        ~Font()
        {
            for (std::map<int, TTF_Font *>::iterator it = cache_.begin(); it != cache_.end(); ++it)
                if (it->second)
                    TTF_CloseFont(it->second);
        }
        bool loadFromFile(const std::string &path)
        {
            if (!TTF_WasInit() && TTF_Init() != 0)
                return false;
            path_ = path;
            return get(20) != nullptr;
        }
        TTF_Font *get(int size) const
        {
            std::map<int, TTF_Font *>::iterator it = cache_.find(size);
            if (it != cache_.end())
                return it->second;
            TTF_Font *f = TTF_OpenFont(path_.c_str(), size);
            cache_[size] = f;
            return f;
        }

    private:
        std::string path_;
        mutable std::map<int, TTF_Font *> cache_;
    };

    class Text
    {
    public:
        enum Style
        {
            Regular = 0,
            Bold = 1
        };
        Text() : font_(nullptr), size_(30), style_(0), x_(0.f), y_(0.f) {}
        void setFont(const Font &f) { font_ = &f; }
        void setCharacterSize(unsigned int s) { size_ = s; }
        void setFillColor(const Color &c) { color_ = c; }
        void setStyle(unsigned int s) { style_ = s; }
        void setString(const std::string &s) { str_ = s; }
        void setPosition(float x, float y)
        {
            x_ = x;
            y_ = y;
        }

        FloatRect getLocalBounds() const
        {
            FloatRect r = {0.f, 0.f, 0.f, 0.f};
            TTF_Font *f = prepare();
            if (!f || str_.empty())
                return r;
            int w = 0, h = 0;
            TTF_SizeUTF8(f, str_.c_str(), &w, &h);
            r.width = (float)w;
            r.height = (float)h;
            return r;
        }

        void drawTo(SDL_Renderer *ren) const
        {
            TTF_Font *f = prepare();
            if (!f || str_.empty())
                return;
            SDL_Color c = {color_.r, color_.g, color_.b, color_.a};
            SDL_Surface *s = TTF_RenderUTF8_Blended(f, str_.c_str(), c);
            if (!s)
                return;
            SDL_Texture *t = SDL_CreateTextureFromSurface(ren, s);
            if (t)
            {
                SDL_Rect d = {(int)x_, (int)y_, s->w, s->h};
                SDL_RenderCopy(ren, t, nullptr, &d);
                SDL_DestroyTexture(t);
            }
            SDL_FreeSurface(s);
        }

    private:
        TTF_Font *prepare() const
        {
            if (!font_)
                return nullptr;
            TTF_Font *f = font_->get((int)size_);
            if (f)
                TTF_SetFontStyle(f, (style_ & Bold) ? TTF_STYLE_BOLD : TTF_STYLE_NORMAL);
            return f;
        }

        const Font *font_;
        unsigned int size_;
        Color color_;
        unsigned int style_;
        float x_, y_;
        std::string str_;
    };

    class RectangleShape
    {
    public:
        explicit RectangleShape(const Vector2f &size) : size_(size), x_(0.f), y_(0.f) {}
        void setPosition(float x, float y)
        {
            x_ = x;
            y_ = y;
        }
        void setFillColor(const Color &c) { fill_ = c; }
        void drawTo(SDL_Renderer *ren) const
        {
            SDL_SetRenderDrawColor(ren, fill_.r, fill_.g, fill_.b, fill_.a);
            SDL_FRect r = {x_, y_, size_.x, size_.y};
            SDL_RenderFillRectF(ren, &r);
        }

    private:
        Vector2f size_;
        float x_, y_;
        Color fill_;
    };

    class CircleShape
    {
    public:
        explicit CircleShape(float radius = 0.f) : r_(radius), ox_(0.f), oy_(0.f), x_(0.f), y_(0.f), thick_(0.f) {}
        float getRadius() const { return r_; }
        void setOrigin(float x, float y)
        {
            ox_ = x;
            oy_ = y;
        }
        void setPosition(float x, float y)
        {
            x_ = x;
            y_ = y;
        }
        void setFillColor(const Color &c) { fill_ = c; }
        void setOutlineColor(const Color &c) { outline_ = c; }
        void setOutlineThickness(float t) { thick_ = t; }

        // The outline is drawn first as a bigger disc, then the fill disc on top.
        // This matches SFML as long as the fill is opaque (it is, in this game).
        void drawTo(SDL_Renderer *ren) const
        {
            float cx = x_ - ox_ + r_;
            float cy = y_ - oy_ + r_;
            if (thick_ > 0.f)
                disc(ren, cx, cy, r_ + thick_, outline_);
            disc(ren, cx, cy, r_, fill_);
        }

    private:
        static void disc(SDL_Renderer *ren, float cx, float cy, float R, const Color &c)
        {
            SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a);
            int y0 = (int)std::floor(cy - R);
            int y1 = (int)std::ceil(cy + R);
            for (int y = y0; y <= y1; ++y)
            {
                float dy = ((float)y + 0.5f) - cy;
                float d = R * R - dy * dy;
                if (d <= 0.f)
                    continue;
                float hw = std::sqrt(d);
                SDL_FRect row = {cx - hw, (float)y, 2.f * hw, 1.f};
                SDL_RenderFillRectF(ren, &row);
            }
        }

        float r_, ox_, oy_, x_, y_, thick_;
        Color fill_, outline_;
    };

    class RenderWindow
    {
    public:
        RenderWindow(const VideoMode &mode, const std::string &title) : win_(nullptr), ren_(nullptr), open_(true)
        {
            SDL_Init(SDL_INIT_VIDEO);
            win_ = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, (int)mode.width, (int)mode.height, 0);
            ren_ = SDL_CreateRenderer(win_, -1, 0);
            SDL_SetRenderDrawBlendMode(ren_, SDL_BLENDMODE_BLEND);
        }
        ~RenderWindow()
        {
            if (ren_)
                SDL_DestroyRenderer(ren_);
            if (win_)
                SDL_DestroyWindow(win_);
        }

        bool isOpen() const { return open_; }
        void close() { open_ = false; }
        void setFramerateLimit(unsigned int) {} // the browser paces frames itself

        bool pollEvent(Event &e)
        {
            SDL_Event se;
            while (SDL_PollEvent(&se))
            {
                if (se.type == SDL_QUIT)
                {
                    e.type = Event::Closed;
                    return true;
                }
                if (se.type == SDL_KEYDOWN)
                {
                    Keyboard::Key k = mapKey(se.key.keysym.sym);
                    if (k != Keyboard::Unknown)
                    {
                        e.type = Event::KeyPressed;
                        e.key.code = k;
                        return true;
                    }
                }
            }
            return false;
        }

        void clear(const Color &c)
        {
            SDL_SetRenderDrawColor(ren_, c.r, c.g, c.b, c.a);
            SDL_RenderClear(ren_);
        }
        void display() { SDL_RenderPresent(ren_); }

        void draw(const RectangleShape &s) { s.drawTo(ren_); }
        void draw(const CircleShape &s) { s.drawTo(ren_); }
        void draw(const Text &t) { t.drawTo(ren_); }
        void draw(const Vertex *v, std::size_t count, PrimitiveType)
        {
            for (std::size_t i = 0; i + 1 < count; i += 2)
            {
                SDL_SetRenderDrawColor(ren_, v[i].color.r, v[i].color.g, v[i].color.b, v[i].color.a);
                SDL_RenderDrawLineF(ren_, v[i].position.x, v[i].position.y, v[i + 1].position.x, v[i + 1].position.y);
            }
        }

    private:
        static Keyboard::Key mapKey(SDL_Keycode k)
        {
            switch (k)
            {
            case SDLK_a:
                return Keyboard::A;
            case SDLK_d:
                return Keyboard::D;
            case SDLK_p:
                return Keyboard::P;
            case SDLK_s:
                return Keyboard::S;
            case SDLK_w:
                return Keyboard::W;
            case SDLK_LEFT:
                return Keyboard::Left;
            case SDLK_RIGHT:
                return Keyboard::Right;
            case SDLK_UP:
                return Keyboard::Up;
            case SDLK_DOWN:
                return Keyboard::Down;
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                return Keyboard::Enter;
            default:
                return Keyboard::Unknown;
            }
        }

        SDL_Window *win_;
        SDL_Renderer *ren_;
        bool open_;
    };

} // namespace sf