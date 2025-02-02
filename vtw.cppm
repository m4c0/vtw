export module vtw;
import dotz;
import hai;
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
      if (codepoint < 0) return false;
      if (codepoint > m_map.size()) return false;
      return m_map[codepoint].in_use;
    }
    [[nodiscard]] const glyph &operator[](int codepoint) const {
      static glyph null{};
      if (codepoint < 0) return null;
      return m_map[codepoint];
    }
    [[nodiscard]] glyph &operator[](int codepoint) {
      static glyph null{};
      if (codepoint < 0) return null = {};
      if (codepoint > m_map.size()) m_map.set_capacity(codepoint + 1);
      m_map[codepoint].in_use = true;
      return m_map[codepoint];
    }
  };
  class atlas {
    voo::h2l_image m_atlas;
    vee::sampler m_smp = vee::create_sampler(vee::linear_sampler);
    voo::single_dset m_dset { vee::dsl_fragment_sampler(), vee::combined_image_sampler() };;
  
  public:
    explicit atlas(vee::physical_device pd) : m_atlas{{ pd, 1024, 1024, vee::image_format_r8, true }} {
      vee::update_descriptor_set(m_dset.descriptor_set(), 0, m_atlas.iv(), *m_smp);
    }
  
    void allocate_glyphs(const wtf::buffer &s, glyphmap &gmap) {
      voo::mapmem m{m_atlas.host_memory()};
      auto charmap = static_cast<unsigned char *>(*m);
  
      int px{};
      int py{};
      int max_h{};
  
      for (auto g : s.glyphs()) {
        if (gmap.exists(g.codepoint())) continue;
  
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
  
    [[nodiscard]] constexpr auto descriptor_set() const { return m_dset.descriptor_set(); }
    [[nodiscard]] constexpr auto descriptor_set_layout() const { return m_dset.descriptor_set_layout(); }
    void setup_copy(vee::command_buffer cb) const { m_atlas.setup_copy(cb); }
    void clear_host(vee::command_buffer cb) const { m_atlas.clear_host(cb); }
  };
  export class scriber {
    vtw::glyphmap m_gmap{};
    vtw::atlas m_a;
    dotz::vec2 m_bounds{};
    dotz::vec2 m_pen{};
  
  public:
    scriber(vee::physical_device pd) : m_a{pd} {}
  
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
  
    [[nodiscard]] constexpr auto descriptor_set() const {
      return m_a.descriptor_set();
    }
    [[nodiscard]] constexpr auto descriptor_set_layout() const {
      return m_a.descriptor_set_layout();
    }
  
    void setup_copy(vee::command_buffer cb) { m_a.setup_copy(cb); }
    void clear_host(vee::command_buffer cb) const { m_a.clear_host(cb); }
  };
}

