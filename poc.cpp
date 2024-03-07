#pragma leco app
#pragma leco add_resource "VictorMono-Regular.otf"
export module poc;

import casein;
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

    quack::instance_batch ib{ps.create_batch(1)};
    ib.map_all([](auto p) {
      auto &[cs, ms, ps, us] = p;
      ps[0] = {{0, 0}, {1, 1}};
      cs[0] = {};
      us[0] = {{0, 0}, {1, 1}};
      ms[0] = {1, 1, 1, 1};
    });

    voo::h2l_image a{dq, 1024, 1024, false};
    auto smp = vee::create_sampler(vee::nearest_sampler);
    auto dset = ps.allocate_descriptor_set(a.iv(), *smp);

    {
      struct glyph {
        unsigned px{};
        unsigned py{};
        wtf::rect rect;
      };
      unsigned charid[10240]; // TODO: max(codepoint) or hashmap
      unsigned curid{};
      glyph gl{};

      voo::mapmem m{a.host_memory()};
      auto charmap = static_cast<unsigned char *>(*m);

      auto s = g_face.shape_en(lorem);
      for (auto g : s.glyphs()) {
        auto &id = charid[g.codepoint()];
        if (id > 0)
          continue;

        g.load_glyph();
        auto [x, y, w, h] = gl.rect = g.bitmap_rect();
        if (gl.px + w + 2 > 1024) { // half width forces line break
          gl.px = 0;
          gl.py += font_h; // TODO: max(h + 2)
        }

        id = ++curid;
        g.blit(charmap, 1024, 1024, gl.px - x + 1, gl.py + y + 1);
        gl.px += w + 2;
      }
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
          ps.run(*scb, 1);
        });
      });
    }
  }
};

extern "C" void casein_handle(const casein::event &e) {
  static renderer r{};
  r.handle(e);
}
