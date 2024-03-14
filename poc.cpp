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
static wtf::face g_face = g_library.new_face("Vazirmatn-Regular.ttf", font_h);

constexpr const auto max_batches = 100;
class renderer : public voo::casein_thread {
public:
  void run() override {
    voo::device_and_queue dq{"quack", native_ptr()};

    quack::pipeline_stuff ps{dq, max_batches};

    quack::instance_batch ib{ps.create_batch(lorem.size())};

    vtw::glyphmap gmap{};
    vtw::atlas a{dq.physical_device(), ps.allocate_descriptor_set()};

    auto s = g_face.shape_en(lorem);
    a.allocate_glyphs(s, gmap);

    const auto scribe = [&s, &gmap](float line_h, auto fn) {
      dotz::vec2 pen{0.0, line_h};

      for (auto g : s.glyphs()) {
        const auto &gl = gmap[g.codepoint()];
        fn(pen, gl);

        pen.x += g.x_advance();
        if (pen.x > 1024.0) {
          pen.x = 0;
          pen.y += line_h * 1.5;
        }
      }
    };

    ib.map_all([&scribe](auto p) {
      constexpr const auto font_hf = static_cast<float>(font_h);
      constexpr const auto line_h = font_hf;
      // This is obviously "1" in this example, but illustrates how we need to
      // consider both variables if they differ for any reason.
      constexpr const auto ratio = line_h / font_hf;

      auto &[cs, ms, ps, us] = p;
      scribe(line_h, [&](auto pen, const auto &gl) {
        auto d = (pen + gl.d) * ratio;
        auto s = gl.size * ratio;
        auto uv0 = gl.uv.xy();
        auto uv1 = uv0 + gl.uv.zw();

        *ps++ = {{d.x, d.y}, {s.x, s.y}};
        *cs++ = {0, 0, 0, 0};
        *us++ = {{uv0.x, uv0.y}, {uv1.x, uv1.y}};
        *ms++ = {1, 1, 1, 1};
      });
    });

    while (!interrupted()) {
      voo::swapchain_and_stuff sw{dq};

      quack::upc rpc{
          .grid_pos = {512.0f, 512.0f},
          .grid_size = {1024.0f, 1024.0f},
      };

      extent_loop(dq.queue(), sw, [&] {
        auto upc = quack::adjust_aspect(rpc, sw.aspect());
        sw.queue_one_time_submit(dq.queue(), [&](auto pcb) {
          ib.setup_copy(*pcb);
          a.setup_copy(*pcb);

          auto scb = sw.cmd_render_pass(pcb);
          vee::cmd_set_viewport(*scb, sw.extent());
          vee::cmd_set_scissor(*scb, sw.extent());
          ib.build_commands(*scb);
          ps.cmd_bind_descriptor_set(*scb, a.descriptor_set());
          ps.cmd_push_vert_frag_constants(*scb, upc);
          ps.run(*scb, lorem.size());
        });
      });
    }
  }
};

extern "C" void casein_handle(const casein::event &e) {
  static renderer r{};
  r.handle(e);
}
