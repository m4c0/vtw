#pragma leco app
#pragma leco add_resource "Vazirmatn-Regular.ttf"
export module poc;

import casein;
import dotz;
import hai;
import jute;
import quack;
import vee;
import voo;
import what_the_font;

namespace vtw {
struct glyph {
  dotz::vec2 d{};
  dotz::vec2 size{};
  dotz::vec4 uv{};
};
struct slot : glyph {
  bool in_use{};
};
class glyphmap {
  static constexpr const auto initial_cap = 1024;
  hai::array<vtw::slot> m_map{initial_cap};

public:
  [[nodiscard]] bool exists(int codepoint) {
    if (codepoint < 0)
      return false;
    if (codepoint > m_map.size())
      return false;
    return m_map[codepoint].in_use;
  }
  [[nodiscard]] const glyph &operator[](int codepoint) const {
    static glyph null{};
    if (codepoint < 0)
      return null;
    return m_map[codepoint];
  }
  [[nodiscard]] glyph &operator[](int codepoint) {
    static glyph null{};
    if (codepoint < 0)
      return null = {};
    if (codepoint > m_map.size())
      m_map.set_capacity(codepoint + 1);
    m_map[codepoint].in_use = true;
    return m_map[codepoint];
  }
};
class atlas {
  voo::h2l_image m_atlas;
  vee::sampler m_smp = vee::create_sampler(vee::linear_sampler);
  vee::descriptor_set m_dset;

public:
  explicit atlas(vee::physical_device pd, vee::descriptor_set dset)
      : m_atlas{pd, 1024, 1024, false}
      , m_dset{dset} {
    vee::update_descriptor_set(dset, 0, m_atlas.iv(), *m_smp);
  }

  void allocate_glyphs(const wtf::buffer &s, glyphmap &gmap) {
    voo::mapmem m{m_atlas.host_memory()};
    auto charmap = static_cast<unsigned char *>(*m);

    int px{};
    int py{};
    int max_h{};

    for (auto g : s.glyphs()) {
      if (gmap.exists(g.codepoint()))
        continue;

      auto &gl = gmap[g.codepoint()];

      g.load_glyph();
      auto [x, y, w, h] = g.bitmap_rect();
      max_h = h > max_h ? h : max_h;
      if (px + w + 2 > 1024) {
        px = 0;
        py += max_h + 2;
        max_h = 0;
      }
      // TODO: check py overflow

      gl.d = dotz::vec2{x, -y};
      gl.size = dotz::vec2{w, h};
      gl.uv = dotz::vec4{px + 1, py + 1, w, h} / 1024.0;

      g.blit(charmap, 1024, 1024, px - x + 1, py + y + 1);
      px += w + 2;
    }
  }

  [[nodiscard]] constexpr auto descriptor_set() const noexcept {
    return m_dset;
  }
  void setup_copy(vee::command_buffer cb) const { m_atlas.setup_copy(cb); }
};
class scriber {
  vtw::glyphmap m_gmap{};
  vtw::atlas m_a;
  dotz::vec2 m_bounds{};
  dotz::vec2 m_pen{};

public:
  scriber(vee::physical_device pd, vee::descriptor_set dset) : m_a{pd, dset} {}

  constexpr void bounds(dotz::vec2 b) { m_bounds = b; }
  constexpr void pen(dotz::vec2 p) { m_pen = p; }

  void draw(wtf::buffer s, auto fn) {
    m_a.allocate_glyphs(s, m_gmap);
    for (auto g : s.glyphs()) {
      const auto &gl = m_gmap[g.codepoint()];
      fn(m_pen, gl);

      m_pen.x += g.x_advance();
      m_pen.y += g.y_advance();
    }
  }

  [[nodiscard]] constexpr auto descriptor_set() const noexcept {
    return m_a.descriptor_set();
  }

  void setup_copy(vee::command_buffer cb) { m_a.setup_copy(cb); }
};
} // namespace vtw

static constexpr const jute::view lorem{
    "Enim nostrum omnis dolorum non unde est velit et. Doloribus fugiat "
    "nesciunt delectus debitis ut dolorum at. Illo veniam laboriosam porro "
    "dolorem dolor. Aliquam eius eos velit. Reprehenderit numquam minima "
    "fugiat nostrum non. Ipsa quia odit voluptatem voluptas beatae. "

    "Est est possimus consequatur quidem. Aut aut quo aliquid veritatis "
    "consequuntur deserunt perspiciatis possimus. Dolorem qui laboriosam "
    "maiores ipsam accusamus voluptas rerum. "

    "Quisquam nisi autem consectetur accusantium officiis quos cupiditate. "
    "Suscipit accusamus itaque quibusdam nesciunt sed et eos. Architecto omnis "
    "excepturi excepturi ut dolorum qui distinctio. "

    "Fuga magnam fuga eligendi pariatur nihil pariatur et. Sit dolore nihil "
    "quia quo rerum. Aliquam ut aliquid quia. Et et necessitatibus asperiores. "
    "Natus nihil eum quia nostrum. Quo vel reiciendis aliquid eveniet. "

    "Eveniet velit tempora provident assumenda perspiciatis et. Odio qui est "
    "expedita enim accusamus fuga inventore quia. Laborum iste aut deleniti. "
    "Pariatur est consequatur accusamus quasi dolor."};

static constexpr const auto font_h = 32;

static wtf::library g_library{};
static wtf::face g_face = g_library.new_face("Vazirmatn-Regular.ttf");

constexpr const auto max_batches = 100;
class renderer : public voo::casein_thread {
public:
  void run() override {
    g_face.set_char_size(font_h);

    voo::device_and_queue dq{"quack"};

    quack::pipeline_stuff ps { dq, max_batches };

    vtw::scriber scr { dq.physical_device(), ps.allocate_descriptor_set() };
    scr.bounds({1024, 1024});

    auto s = g_face.shape_en(lorem);

    const auto scribe = [&s, &scr](float line_h, auto fn) {
      scr.pen({0, font_h});
      scr.draw(s, [&](auto pen, const auto &gl) {
        if (pen.x > 1024.0) {
          pen.x = 0;
          pen.y += line_h;
          scr.pen(pen);
        }
        fn(pen, gl);
      });
    };
    const auto update_data = [&scribe](quack::instance *& i) {
      constexpr const auto font_hf = static_cast<float>(font_h);
      // Size of font on screen, in screen units
      constexpr const auto font_scr_h = font_hf;
      // This is obviously "1" in this example, but illustrates how we need to
      // consider both variables if they differ for any reason.
      constexpr const auto ratio = font_scr_h / font_hf;

      scribe(font_hf * 1.5, [&](auto pen, const auto &gl) {
        auto d = (pen + gl.d) * ratio;
        auto s = gl.size * ratio;
        auto uv0 = gl.uv.xy();
        auto uv1 = uv0 + gl.uv.zw();

        *i++ = (quack::instance) {
          .position   = d,
          .size       = s,
          .uv0        = uv0,
          .uv1        = uv1,
          .colour     = {},
          .multiplier = { 1, 1, 1, 1 },
        };
      });
    };

    quack::buffer_updater u { &dq, lorem.size(), update_data };
    u.run_once();

    while (!interrupted()) {
      voo::swapchain_and_stuff sw{dq};

      quack::upc rpc{
          .grid_pos = {512.0f, 512.0f},
          .grid_size = {1024.0f, 1024.0f},
      };

      extent_loop(dq.queue(), sw, [&] {
        sw.queue_one_time_submit(dq.queue(), [&](auto pcb) {
          scr.setup_copy(*pcb);

          auto scb = sw.cmd_render_pass({
              .command_buffer = *pcb,
              .clear_colours { vee::clear_colour({}) },
          });
          quack::run(&ps, {
              .sw = &sw,
              .scb = *scb,
              .pc = &rpc,
              .inst_buffer = u.data().local_buffer(),
              .atlas_dset = scr.descriptor_set(),
              .count = lorem.size(),
          });
        });
      });
    }
  }
} t;

