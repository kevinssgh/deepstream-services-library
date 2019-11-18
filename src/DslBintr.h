/*
The MIT License

Copyright (c) 2019-Present, ROBERT HOWELL

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in-
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef _DSL_BINTR_H
#define _DSL_BINTR_H

#include "Dsl.h"
#include "DslNodetr.h"

namespace DSL
{
    /**
     * @brief convenience macros for shared pointer abstraction
     */
    #define DSL_BINTR_PTR std::shared_ptr<Bintr>
    #define DSL_BINTR_NEW(name) \
        std::shared_ptr<Bintr>(new Bintr(name))    

    /**
     * @class Bintr
     * @brief Implements a base container class for a GST Bin
     */
    class Bintr : public GstNodetr
    {
    public:

        /**
         * @brief named container ctor with new Bin 
         */
        Bintr(const char* name)
            : GstNodetr(name)
            , m_isLinked(false)
            , m_gpuId(0)
            , m_nvbufMemoryType(0)
            , m_pGstStaticSinkPad(NULL)
            , m_pGstStaticSourcePad(NULL)
        { 
            LOG_FUNC(); 

            m_pGstObj = GST_OBJECT(gst_bin_new((gchar*)name));
            if (!m_pGstObj)
            {
                LOG_ERROR("Failed to create a new GST bin for Bintr '" << name << "'");
                throw;  
            }
        }
        
        /**
         * @brief Bintr dtor to release all GST references
         */
        ~Bintr()
        {
            LOG_FUNC();
        }

        /**
         * @brief Adds this Bintr as a child to a ParentBinter
         * @param pParentBintr to add to
         */
        virtual bool AddToParent(DSL_NODETR_PTR pParent)
        {
            LOG_FUNC();
                
            return (bool)pParent->AddChild(shared_from_this());
        }
        
        /**
         * @brief removes this Bintr from the provided pParentBintr
         * @param pParentBintr Bintr to remove from
         */
        virtual bool RemoveFromParent(DSL_NODETR_PTR pParentBintr)
        {
            LOG_FUNC();
                
            return pParentBintr->RemoveChild(shared_from_this());
        }
        
        virtual void AddGhostPad(const char* name, DSL_NODETR_PTR pElementr)
        {
            LOG_FUNC();
            
            // create a new ghost pad with the static Sink pad retrieved from this Elementr's 
            // pGstObj and adds it to the the Elementr's Parent Bintr's pGstObj.
            if (!gst_element_add_pad(GetGstElement(), 
                gst_ghost_pad_new(name, gst_element_get_static_pad(pElementr->GetGstElement(), name))))
            {
                LOG_ERROR("Failed to add Pad '" << name << "' for element'" << GetName() << "'");
                throw;
            }
        }

        /**
         * @brief virtual function for derived classes to implement
         * a bintr type specific function to link all children.
         */
        virtual bool LinkAll() = 0;
        
        /**
         * @brief virtual function for derived classes to implement
         * a bintr type specific function to unlink all child elements.
         */
        virtual void UnlinkAll() = 0;
        
        /**
         * @brief called to determine if a Bintr's Child Elementrs are Linked
         * @return true if Child Elementrs are currently Linked, false otherwise
         */
        bool IsLinked()
        {
            LOG_FUNC();
            
            return m_isLinked;
        }

        /**
         * @brief called to determine if a Bintr is currently in use - has a Parent
         * @return true if the Bintr has a Parent, false otherwise
         */
        bool IsInUse()
        {
            LOG_FUNC();
            
            return (bool)GetParentGstElement();
        }

        bool Play()
        {
            LOG_FUNC();
            
            LOG_INFO("Changing state to GST_STATE_PLAY for Bintr '" << GetName() << "'");

            gst_element_set_state(GetGstElement(), GST_STATE_PLAYING);
            
            // Wait until state change or failure, no timeout.
            if (gst_element_get_state(GetGstElement(), NULL, NULL, -1) == GST_STATE_CHANGE_FAILURE)
            {
                LOG_ERROR("FAILURE occured when trying to play Bintr '" << GetName() << "'");
                return false;
            }
            return true;
        }

        bool Pause()
        {
            LOG_FUNC();
            
            LOG_INFO("Changing state to GST_STATE_PAUSED for Bintr '" << GetName() << "'");
            
            gst_element_set_state(GetGstElement(), GST_STATE_PAUSED);

            // Wait until state change or failure, no timeout.
            if (gst_element_get_state(GetGstElement(), NULL, NULL, -1) == GST_STATE_CHANGE_FAILURE)
            {
                LOG_ERROR("FAILURE occured when trying to pause Bintr '" << GetName() << "'");
                return false;
            }
            return true;
        }

        bool Stop()
        {
            LOG_FUNC();
            
            uint currentState = GetState();
            
            if ((currentState != GST_STATE_PLAYING) and (currentState != GST_STATE_PAUSED))
            {
                LOG_ERROR("Bintr '" << GetName() << "' is not in a state of PLAYING or PAUSED");
                return false;
            }
            
            return gst_pad_send_event(
                gst_element_get_static_pad(GetGstElement(), "sink"), gst_event_new_eos());
        }
        
    public:
    
        bool m_isLinked;

        /**
         * @brief
         */
        guint m_gpuId;

        /**
         * @brief
         */
        guint m_nvbufMemoryType;

        /**
         * @brief Static Pad object for the Sink Elementr within this Bintr
         */
        GstPad* m_pGstStaticSinkPad;
            
        /**
         * @brief A dynamic collection of requested Sink Pads for this Bintr
         */
        std::map<std::string, GstPad*> m_pGstRequestedSinkPads;
            
        /**
         * @brief Static Pad object for the Source Elementr within this Bintr
         */
        GstPad* m_pGstStaticSourcePad;
            
        /**
         * @brief A dynamic collection of requested Souce Pads for this Bintr
         */
        std::map<std::string, GstPad*> m_pGstRequestedSourcePads;
    };

} // DSL

#endif // _DSL_BINTR_H