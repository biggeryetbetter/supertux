//  SuperTux - Wind
//  Copyright (C) 2006 Christoph Sommer <christoph.sommer@2006.expires.deltadevelopment.de>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "object/wind.hpp"

#include <simplesquirrel/class.hpp>
#include <simplesquirrel/vm.hpp>

#include "badguy/badguy.hpp"
#include "badguy/dive_mine.hpp"
#include "editor/editor.hpp"
#include "math/random.hpp"
#include "object/particles.hpp"
#include "object/player.hpp"
#include "object/rock.hpp"
#include "object/sprite_particle.hpp"
#include "sprite/sprite.hpp"
#include "sprite/sprite_manager.hpp"
#include "supertux/game_object.hpp"
#include "supertux/sector.hpp"
#include "util/reader_mapping.hpp"
#include "video/drawing_context.hpp"

Wind::Wind(const ReaderMapping& reader) :
  MovingObject(reader),
  blowing(),
  speed(0.0f, 0.0f),
  acceleration(),
  new_size(0.0f, 0.0f),
  dt_sec(0),
  feather_distance(0.f),
  affects_badguys(),
  affects_objects(),
  affects_player(),
  fancy_wind()
{
  float w,h;
  parse_type(reader);
  reader.get("x", m_col.m_bbox.get_left(), 0.0f);
  reader.get("y", m_col.m_bbox.get_top(), 0.0f);
  reader.get("width", w, 32.0f);
  reader.get("height", h, 32.0f);
  m_col.m_bbox.set_size(w, h);

  reader.get("blowing", blowing, true);

  reader.get("feather-distance", feather_distance, 16.f);
  reader.get("speed-x", speed.x, 0.0f);
  reader.get("speed-y", speed.y, 0.0f);

  reader.get("acceleration", acceleration, 100.0f);

  reader.get("affects-badguys", affects_badguys, false);
  reader.get("affects-objects", affects_objects, false);
  reader.get("affects-player", affects_player, true);
  
  reader.get("fancy-wind", fancy_wind, false);

  set_group(COLGROUP_TOUCHABLE);
}

ObjectSettings
Wind::get_settings()
{
  new_size.x = m_col.m_bbox.get_width();
  new_size.y = m_col.m_bbox.get_height();

  ObjectSettings result = MovingObject::get_settings();

  //result.add_float("width", &new_size.x, "width", OPTION_HIDDEN);
  //result.add_float("height", &new_size.y, "height", OPTION_HIDDEN);
  result.add_float(_("Speed X"), &speed.x, "speed-x");
  result.add_float(_("Speed Y"), &speed.y, "speed-y");
  result.add_float(_("Acceleration"), &acceleration, "acceleration");
  result.add_float(_("Feather Distance"), &feather_distance, "feather-distance");
  result.add_bool(_("Blowing"), &blowing, "blowing", true);
  result.add_bool(_("Affects Badguys"), &affects_badguys, "affects-badguys", false);
  result.add_bool(_("Affects Objects"), &affects_objects, "affects-objects", false);
  result.add_bool(_("Affects Player"), &affects_player, "affects-player");
  result.add_bool(_("Fancy Particles"), &fancy_wind, "fancy-wind", false);

  result.reorder({"blowing", "speed-x", "speed-y", "acceleration", "feather-distance", "affects-badguys", "affects-objects", "affects-player", "fancy-wind", "region", "name", "x", "y"});

  return result;
}

GameObjectTypes
Wind::get_types() const {
  return {
    { "wind", _("Wind") },
    { "current", _("Current") }
  };
}

void
Wind::update(float dt_sec_)
{
  dt_sec = dt_sec_;

  if (!blowing) return;
  if (m_col.m_bbox.get_width() <= 16 || m_col.m_bbox.get_height() <= 16) return;

  Vector ppos = Vector(graphicsRandom.randf(m_col.m_bbox.get_left()+8, m_col.m_bbox.get_right()-8), graphicsRandom.randf(m_col.m_bbox.get_top()+8, m_col.m_bbox.get_bottom()-8));
  Vector pspeed = Vector(graphicsRandom.randf(speed.x-20, speed.x+20), graphicsRandom.randf(speed.y-20, speed.y+20));

  // TODO: Rotate sprite rather than just use 2 different actions
  // Approx. 1 particle per tile
  if (graphicsRandom.randf(0.f, 100.f) < (m_col.m_bbox.get_width() / 32.f) * (m_col.m_bbox.get_height() / 32.f))
  {
    // emit a particle
	  if (fancy_wind)
    {
      switch (m_type) {
        case WIND: // Normal wind
          Sector::get().add<SpriteParticle>("images/particles/wind.sprite", (std::abs(speed.x) > std::abs(speed.y)) ? "default" : "flip", ppos, ANCHOR_MIDDLE, pspeed, Vector(0, 0), LAYER_BACKGROUNDTILES + 1);
          break;
        case CURRENT: // Current variant
          Sector::get().add<SpriteParticle>("images/particles/water_piece1.sprite", (std::abs(speed.x) > std::abs(speed.y)) ? "default" : "flip", ppos, ANCHOR_MIDDLE, pspeed, Vector(0, 0), LAYER_BACKGROUNDTILES + 1);
          break;
      }
	  }
	  else
    {
	    Sector::get().add<Particles>(ppos, 44, 46, pspeed, Vector(0, 0), 1, Color(.4f, .4f, .4f), 3, .1f, LAYER_BACKGROUNDTILES + 1);
	  }
  }
}

void
Wind::draw(DrawingContext& context)
{
  if (Editor::is_active()) {
    context.color().draw_filled_rect(m_col.m_bbox, Color(0.0f, 1.0f, 1.0f, 0.6f),
                             0.0f, LAYER_OBJECTS);
  }
}

HitResponse
Wind::collision(GameObject& other, const CollisionHit& )
{
  if (!blowing) return ABORT_MOVE;

  auto player = dynamic_cast<Player*> (&other);
  if (player && affects_player)
  {
    player->override_velocity();
    player->add_wind_velocity(speed, acceleration, speed * get_wind_strength(player->get_bbox().get_middle()));
  }

  auto badguy = dynamic_cast<BadGuy*>(&other);
  if (badguy && affects_badguys && badguy->can_be_affected_by_wind())
  {
    if (m_type == CURRENT && dynamic_cast<DiveMine*>(badguy)) { // Dive mines are not affected by currents
      return ABORT_MOVE;
    }
    badguy->add_wind_velocity(speed * get_wind_strength(badguy->get_bbox().get_middle()) * acceleration * dt_sec, speed);
  }

  auto rock = dynamic_cast<Rock*>(&other);
  if (rock && affects_objects)
  {
    rock->add_wind_velocity(speed * get_wind_strength(rock->get_bbox().get_middle()) * acceleration * dt_sec, speed);
  }

  return ABORT_MOVE;
}

void
Wind::start()
{
  blowing = true;
}

void
Wind::stop()
{
  blowing = false;
}

void
Wind::on_flip(float height)
{
  MovingObject::on_flip(height);
  speed.y = -speed.y;
}

float
Wind::get_wind_strength(Vector pos) {
  // Point isn't inside the wind
  if (!m_col.m_bbox.contains(pos))
    return 0.0;

  float strength = 1.0;


  float dl = pos.x - m_col.m_bbox.get_left();
  float dr = m_col.m_bbox.get_right() - pos.x;
  float dt = pos.y - m_col.m_bbox.get_top();
  float db = m_col.m_bbox.get_bottom() - pos.y;

  strength = std::clamp(std::min({dl, dr, dt, db}) / feather_distance, 0.f, 1.f);

  return strength;
}

void
Wind::register_class(ssq::VM& vm)
{
  ssq::Class cls = vm.addAbstractClass<Wind>("Wind", vm.findClass("MovingObject"));

  cls.addFunc("start", &Wind::start);
  cls.addFunc("stop", &Wind::stop);
}

/* EOF */
