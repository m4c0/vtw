#pragma leco app
#pragma leco add_resource "Vazirmatn-Regular.ttf"
export module poc;

import casein;
import jute;
import quack;
import vapp;
import vee;
import voo;
import vtw;
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
static wtf::face g_face = g_library.new_face("Vazirmatn-Regular.ttf");

constexpr const auto max_batches = 100;
class renderer : public vapp {
public:
  void run() override {
    g_face.set_char_size(font_h);

    voo::device_and_queue dq { "quack", casein::native_ptr };

    quack::pipeline_stuff ps { dq, max_batches };

    vtw::scriber scr { dq.physical_device() };
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

