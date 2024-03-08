#pragma leco app
#pragma leco add_resource "VictorMono-Regular.otf"
export module poc;

import casein;
import dotz;
import jute;
import quack;
import vee;
import voo;
import what_the_font;

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
static wtf::face g_face = g_library.new_face("VictorMono-Regular.otf", font_h);

constexpr const auto max_batches = 100;
class renderer : public voo::casein_thread {
public:
  void run() override {
    voo::device_and_queue dq{"quack", native_ptr()};

    quack::pipeline_stuff ps{dq, max_batches};

    quack::instance_batch ib{ps.create_batch(lorem.size())};

    voo::h2l_image a{dq, 1024, 1024, false};
    auto smp = vee::create_sampler(vee::nearest_sampler);
    auto dset = ps.allocate_descriptor_set(a.iv(), *smp);

    {
      struct glyph {
        dotz::vec2 d{};
        dotz::vec2 size{};
        dotz::vec4 uv{};
        bool in_use{};
      };
      glyph charid[10240]; // TODO: max(codepoint) or hashmap
      int px{};
      int py{};

      voo::mapmem m{a.host_memory()};
      auto charmap = static_cast<unsigned char *>(*m);

      auto s = g_face.shape_en(lorem);
      for (auto g : s.glyphs()) {
        auto &gl = charid[g.codepoint()];
        if (gl.in_use > 0)
          continue;

        g.load_glyph();
        auto [x, y, w, h] = g.bitmap_rect();
        if (px + w + 2 > 1024) {
          px = 0;
          py += font_h; // TODO: max(h + 2)
        }
        // TODO: check py overflow

        constexpr const auto font_hf = static_cast<float>(font_h);
        gl.d = dotz::vec2{x, -y} / font_hf;
        gl.size = dotz::vec2{w, h} / font_hf;
        gl.uv = dotz::vec4{px, py, w, h} / 1024.0;
        gl.in_use = true;

        g.blit(charmap, 1024, 1024, px - x + 1, py + y + 1);
        px += w + 2;
      }

      ib.map_all([&](auto p) {
        constexpr const auto line_h = 0.05;
        float px{0};
        float py{line_h};

        auto &[cs, ms, ps, us] = p;
        for (auto g : s.glyphs()) {
          const auto &gl = charid[g.codepoint()];
          auto d = gl.d * line_h;
          auto s = gl.size * line_h;
          auto uv0 = gl.uv.xy();
          auto uv1 = uv0 + gl.uv.zw();

          *ps++ = {{px + d.x, py + d.y}, {s.x, s.y}};
          *cs++ = {0, 0, 0, 1};
          *us++ = {{uv0.x, uv0.y}, {uv1.x, uv1.y}};
          *ms++ = {1, 1, 1, 1};

          px += g.x_advance() * line_h / static_cast<float>(font_h);
          if (px > 1.0) {
            px = 0;
            py += line_h;
          }
        }
      });
    }

    while (!interrupted()) {
      voo::swapchain_and_stuff sw{dq};

      quack::upc rpc{
          .grid_pos = {0.5f, 0.5f},
          .grid_size = {1.0f, 1.0f},
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
          ps.cmd_bind_descriptor_set(*scb, dset);
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
