/***************************************************************************
 ** This file is part of the generic algorithm library Wiselib.           **
 ** Copyright (C) 2008,2009 by the Wisebed (www.wisebed.eu) project.      **
 **                                                                       **
 ** The Wiselib is free software: you can redistribute it and/or modify   **
 ** it under the terms of the GNU Lesser General Public License as        **
 ** published by the Free Software Foundation, either version 3 of the    **
 ** License, or (at your option) any later version.                       **
 **                                                                       **
 ** The Wiselib is distributed in the hope that it will be useful,        **
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of        **
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
 ** GNU Lesser General Public License for more details.                   **
 **                                                                       **
 ** You should have received a copy of the GNU Lesser General Public      **
 ** License along with the Wiselib.                                       **
 ** If not, see <http://www.gnu.org/licenses/>.                           **
 ***************************************************************************/

#ifndef SELF_STABILIZING_TREE_H
#define SELF_STABILIZING_TREE_H

#include <util/serialization/serialization.h>
#include <algorithms/semantic_entities/token_construction/nap_control.h>
#include "tree_state_message.h"
#include <algorithms/semantic_entities/token_construction/regular_event.h>
#include <util/types.h>

namespace wiselib {
	
	/**
	 * @brief
	 * 
	 * @ingroup
	 * 
	 * @tparam 
	 */
	template<
		typename OsModel_P,
		typename UserData_P,
		typename Radio_P,
		typename Clock_P,
		typename Timer_P,
		typename NapControl_P = NapControl<OsModel_P, Radio_P>
	>
	class SelfStabilizingTree {
		public:
			typedef SelfStabilizingTree self_type;
			typedef self_type* self_pointer_t;
				
			typedef OsModel_P OsModel;
			typedef typename OsModel::block_data_t block_data_t;
			typedef typename OsModel::size_t size_type;
			typedef Radio_P Radio;
			typedef typename Radio::node_id_t node_id_t;
			typedef typename Radio::message_id_t message_id_t;
			typedef Clock_P Clock;
			typedef typename Clock::time_t time_t;
			typedef ::uint32_t abs_millis_t;
			typedef Timer_P Timer;
			typedef NapControl_P NapControlT;
			typedef UserData_P UserData;
			typedef TreeStateMessage<OsModel, Radio, UserData> TreeStateMessageT;
			typedef typename TreeStateMessageT::TreeStateT TreeStateT;
			typedef RegularEvent<OsModel, Radio, Clock, Timer> RegularEventT;
			
			enum State { IN_EDGE = 1, OUT_EDGE = 2, BIDI_EDGE = IN_EDGE | OUT_EDGE  };
			enum {
				BCAST_INTERVAL = 200,
				DEAD_INTERVAL = 2 * BCAST_INTERVAL
			};
			enum { NULL_NODE_ID = Radio::NULL_NODE_ID, BROADCAST_ADDRESS = Radio::BROADCAST_ADDRESS };
			enum { npos = (size_type)(-1) };
			enum { MAX_NEIGHBORS = 16 };
			enum EventType {
				NEW_NEIGHBOR, LOST_NEIGHBOR, UPDATED_NEIGHBOR
			};
			
			typedef delegate1<void, EventType> event_callback_t;
			
			struct NeighborEntry {
				// {{{
				NeighborEntry() : address_(NULL_NODE_ID) {
				}
				
				void from_message(node_id_t addr, const TreeStateMessageT& m, abs_millis_t t) {
					message_ = m;
					last_update_ = t;
					address_ = addr;
				}
				
				bool used() {
					return address_ != NULL_NODE_ID;
				}
				
				TreeStateMessageT message_;
				abs_millis_t last_update_;
				node_id_t address_;
				// }}}
			};
			
			/**
			 * @ingroup Neighbor_concept
			 */
			class Neighbor {
				// {{{
				public:
					node_id_t id() { return id_; }
					State state() { return state_; }
					
					node_id_t id_;
					State state_;
				// }}}
			};
			
			class iterator {
				// {{{
				public:
					iterator(NeighborEntry* neighbors, size_type index)
						: neighbors_(neighbors), index_(index) {
						update();
					}
					
					Neighbor operator*() {
						return neighbor_;
					}
					
					bool at_end() {
						return index_ == npos;
					}
					
					iterator& operator++() {
						if(index_ < MAX_NEIGHBORS - 1) {
							index_ = npos;
						}
						else {
							index_++;
						}
						update();
					}
					
					void update() {
						if(index_ == npos) {
							neighbor_.address_ = NULL_NODE_ID;
						}
						else {
							neighbor_.address_ = neighbors_[index_].address_;
							neighbor_.state_ = (index_ == 0) ? OUT_EDGE : IN_EDGE;
						}
						if(neighbor_.address_ == NULL_NODE_ID) {
							index_ = npos;
						}
					}
					
				private:
					NeighborEntry *neighbors_;
					size_type index_;
					Neighbor neighbor_;
				// }}}
			}; // iterator
			
			void init(typename Radio::self_pointer_t radio, typename Clock::self_pointer_t clock, typename Timer::self_pointer_t timer, typename NapControlT::self_pointer_t nap_control) {
				radio_ = radio;
				radio_->template reg_recv_callback<self_type, &self_type::on_receive>(this);
				clock_ = clock;
				timer_ = timer;
				new_neighbors_ = false;
				lost_neighbors_ = false;
				nap_control_ = nap_control;
			}
			
			void reg_event_callback(event_callback_t cb) {
				event_callback_ = cb;
			}
			
			iterator begin_neighbors() {
				return iterator(neighbors_, 0);
			}
			
			iterator end_neighbors() {
				return iterator(neighbors_, npos);
			}
			
			node_id_t parent() {
				return neighbors_[0].address_;
			}
			
			node_id_t child(size_type idx) {
				return neighbors_[idx + 1].address_;
			}
			
			UserData child_user_data(size_type idx) {
				return neighbors_[idx + 1].message_.user_data();
			}
			
			TreeStateT state() {
				return tree_state_message_.tree_state();
			}
			
			/**
			 * @return The number of childs with a lower node id than n.
			 */
			size_type child_index(node_id_t n) {
				size_type idx = find_neighbor_position(n);
				return (neighbors_[idx].address_ == n) ? idx : npos;
			}
			
			size_type childs() {
				return (parent() == NULL_NODE_ID) ? neighbors_count_ : neighbors_count_ - 1;
			}
			
			size_type size() {
				return neighbors_count_;
			}
			
		
		private:
			
			void broadcast_state() {
				nap_control_->push_caffeine();
				radio_->send(BROADCAST_ADDRESS, tree_state_message_.size(), tree_state_message_.data());
				nap_control_->pop_caffeine();
			}
			
			void on_receive(node_id_t from, typename Radio::size_t len, block_data_t* data) {
				message_id_t message_type = wiselib::read<OsModel, block_data_t, message_id_t>(data);
				if(message_type != TreeStateMessageT::MESSAGE_TYPE) {
					return;
				}
				
				TreeStateMessageT &msg = *reinterpret_cast<TreeStateMessageT*>(data);
				abs_millis_t t_recv = now();
				msg.check();
				
				if(msg.reason() == TreeStateMessageT::REASON_REGULAR_BCAST) {
					RegularEventT &event = regular_broadcasts_[from];
					event.hit(t_recv, clock_, radio_->id());
					event.end_waiting();
					
					void *v;
					hardcore_cast(v, from);
					event.template start_waiting_timer<
						self_type, &self_type::begin_wait_for_regular_broadcast,
						&self_type::end_wait_for_regular_broadcast>(clock_, timer_, this, v);
				}
				
				add_neighbor(from, msg);
			}
			
			void begin_wait_for_regular_broadcast(void*) {
				// TODO
			}
			
			void end_wait_for_regular_broadcast(void*) {
				// TODO
			}
			
			size_type find_neighbor_position(node_id_t a) {
				if(neighbors_[0].address_ == a) { return 0; }
				
				node_id_t l = 1;
				node_id_t r = neighbors_count_;
				
				while(l + 1 < r) {
					node_id_t m = (l + r) / 2;
					if(neighbors_[m].address_ == a) { return m; }
					else if(neighbors_[m].address_ < a) { l = m; }
					else { r = m; }
				}
				if(neighbors_[l].address_ == a) { return l; }
				//if(neighbors_[r].address_ == a) { return r; }
				//return npos;
				return r;
			}
			
			void add_neighbor(node_id_t addr, const TreeStateMessageT& msg) {
				if(neighbors_count_ == MAX_NEIGHBORS) {
					return;
				}
				
				size_type p = find_neighbor_position(addr);
				if(neighbors_[p].address_ == addr) {
					neighbors_[p].from_message(addr, msg, now());
					
					if(event_callback_) {
						event_callback_(UPDATED_NEIGHBOR);
					}
				}
				else {
					if(p == 0) {
						if(neighbors_count_ > 0) {
							size_type pp = find_neighbor_position(neighbors_[0].address_);
							insert_child(pp, neighbors_[0].address_, neighbors_[0].message_, neighbors_[0].last_update_);
						}
						neighbors_[0].from_message(addr, msg, now());
					}
					else {
						insert_child(p, addr, msg, now());
					}
					
					neighbors_count_++;
					if(event_callback_) {
						event_callback_(NEW_NEIGHBOR);
					}
					new_neighbors_ = true;
				}
			}
			
			void insert_child(size_type p, node_id_t addr, const TreeStateMessageT& msg, abs_millis_t last_update) {
				assert(p > 0);
				assert(p != npos);
				
				if(p < neighbors_count_) {
					memmove(neighbors_ + p + 1, neighbors_ + p, (neighbors_count_ - p)*sizeof(NeighborEntry));
				}
				neighbors_[p].from_message(addr, msg, last_update);
				neighbors_count_++;
			}
			
			void cleanup_dead_neighbors() {
				size_type p = 0;
				while(p < neighbors_count_) {
					if(neighbors_[p].last_update_ + DEAD_INTERVAL <= now()) {
						// TODO: cancel timers where necessary!
						
						//neighbors_[p].address_ = NULL_NODE_ID;
						memmove(neighbors_ + p, neighbors_ + p + 1, (neighbors_count_ - p) * sizeof(NeighborEntry));
						neighbors_count_--;
						lost_neighbors_ = true;
						if(event_callback_) {
							event_callback_(LOST_NEIGHBOR);
						}
					}
					else {
						p++;
					}
				} // while
			}
			
			bool update_state() {
				cleanup_dead_neighbors();
				
				::uint8_t distance = -1;
				node_id_t parent = radio_->id();
				node_id_t root = radio_->id();
				size_type parent_idx = npos;
				
				for(size_type i = 0; i < MAX_NEIGHBORS; i++) {
					NeighborEntry &e = neighbors_[i];
					if(e.state_.parent() == radio_->id()) { continue; }
					if(e.state_.root() == NULL_NODE_ID || e.state_.distance() == (::uint8_t)(-1)) { continue; }
					
					if(e.state_.root() < root) {
						parent = e.address_;
						parent_idx = i;
						root = e.state_.root();
						distance = e.state_.distance() + 1;
					}
					else if(e.state_.root() == root && (e.state_.distance() + 1) < distance) {
						parent = e.address_;
						parent_idx = i;
						distance = e.state_.distance() + 1;
					}
				}
				
				if(root == radio_->id()) {
					distance = 0;
					parent = radio_->id();
					parent_idx = npos;
				}
				
				bool c_a = state().set_distance(distance);
				bool c_b = state().set_parent(parent);
				bool c_c = state().set_root(root);
				bool changed = new_neighbors_ || lost_neighbors_ || c_a || c_b || c_c;
				
				new_neighbors_ = false;
				lost_neighbors_ = false;
				
				return changed;
			}
			
			UserData& user_data() {
				return tree_state_message_.user_data();
			}
			
			void set_user_data(UserData& ud) {
				tree_state_message_.set_user_data(ud);
			}
			
			abs_millis_t absolute_millis(const time_t& t) {
				return clock_->seconds(t) * 1000 + clock_->milliseconds(t);
			}
			
			abs_millis_t now() {
				return absolute_millis(clock_->time());
			}
			
			typename Radio::self_pointer_t radio_;
			typename Clock::self_pointer_t clock_;
			typename Timer::self_pointer_t timer_;
			typename NapControlT::self_pointer_t nap_control_;
			TreeStateMessageT tree_state_message_;
			bool new_neighbors_;
			bool lost_neighbors_;
			
			size_type neighbors_count_;
			NeighborEntry neighbors_[MAX_NEIGHBORS];
			RegularEventT regular_broadcasts_[MAX_NEIGHBORS];
			event_callback_t event_callback_;
		
	}; // SelfStabilizingTree
}

#endif // SELF_STABILIZING_TREE_H

