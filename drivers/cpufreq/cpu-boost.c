/*
 * Copyright (c) 2013-2015, The Linux Foundation. All rights reserved.
 *
 * Dynamic SchedTune Integration
 * Copyright (c) 2020, Edrick Vince Sinsuan <sedrickvince@gmail.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/fb.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/cpu-boost.h>
#include <linux/sched.h>

/*
 * Modifiable Variables
 */
static bool __read_mostly dsboost_input_state = true;
module_param(dsboost_input_state, bool, 0644);

static bool __read_mostly dsboost_kick_state = true;
module_param(dsboost_kick_state, bool, 0644);

static unsigned int __read_mostly input_sched_boost =
	CONFIG_INPUT_SCHED_BOOST;
static unsigned int __read_mostly kick_sched_boost =
	CONFIG_KICK_SCHED_BOOST;
static unsigned short __read_mostly input_duration_ms =
	CONFIG_INPUT_DURATION;
static unsigned short __read_mostly kick_duration_ms =
	CONFIG_KICK_DURATION;

module_param(input_sched_boost, uint, 0644);
module_param(kick_sched_boost, uint, 0644);
module_param(input_duration_ms, ushort, 0644);
module_param(kick_duration_ms, ushort, 0644);

/*
 * Driver variables and structures
 */
struct boost_val {
	bool curr_state;
	struct workqueue_struct *boost_wq;
	struct work_struct enable;
	struct delayed_work disable;
	unsigned short duration, stored_duration_ms;
	unsigned int stored_val;
	int slot;
};

/* Boost value structures */
static struct boost_val input, kick;

/* Framebuffer state notifier and stored blank boolean */
static struct notifier_block fb_notifier;
static bool stored_blank;

/* Function to check cpuboost state for other drivers */
bool cpuboost_state(bool is_input)
{
	return is_input ? dsboost_input_state : dsboost_kick_state;
}

static void set_boost(struct boost_val *boost, bool enable)
{
	if (boost->curr_state == enable)
		return;

	boost->curr_state = enable ?
		!do_stune_boost("top-app", boost->stored_val, &boost->slot) :
			reset_stune_boost("top-app", boost->slot);

	if (boost == &input) {
		/*
		 * Enable boost and prefer_idle with bias function in order to bias 
		 * migrating top-app (also for foreground) tasks to idle big cluster cores.
		 */
		do_prefer_idle("top-app", enable);
		do_prefer_idle("foreground", enable);
	} else {
		/*
		 * Use idle cpus with the highest original capacity for top-app when it
		 * comes to app launches and transitions in order to speed up 
		 * the process and efficiently consume power.
		 */
		do_crucial("top-app", enable);
	}
}

static void trigger_boost(struct boost_val *boost, unsigned int *sched_boost,
		unsigned short *duration_ms)
{
	if (*duration_ms != boost->stored_duration_ms) {
		/* Return stored value if invalid */
		if (*duration_ms < 10) {
			*duration_ms = boost->stored_duration_ms;
		} else {
			boost->stored_duration_ms = *duration_ms;
			boost->duration = msecs_to_jiffies(boost->stored_duration_ms);
		}
	}

	mod_delayed_work(boost->boost_wq, &boost->disable, boost->duration);

	if (*sched_boost != boost->stored_val) {
		/* Return stored value if invalid */
		if (*sched_boost <= 0 || *sched_boost > 100)
			*sched_boost = boost->stored_val;
		else
			boost->stored_val = *sched_boost;

		/* If boost is already active, just update boost value */
		if (boost->curr_state) {
			reset_stune_boost("top-app", boost->slot);
			do_stune_boost("top-app", boost->stored_val, &boost->slot);
			return;
		}
	}

	set_boost(boost, true);
}

static void trigger_input(struct work_struct *work)
{
	trigger_boost(&input, &input_sched_boost, &input_duration_ms);
}

static void trigger_kick(struct work_struct *work)
{
	trigger_boost(&kick, &kick_sched_boost, &kick_duration_ms);
}

static void input_remove(struct work_struct *work)
{
	set_boost(&input, false);
}

static void kick_remove(struct work_struct *work)
{
	set_boost(&kick, false);
}

static void trigger_event(struct boost_val *boost, bool state)
{
	/* Disable boost if state or stored_blank is false */
	if (!state || !stored_blank) {
		if (boost->curr_state)
			mod_delayed_work(boost->boost_wq, &boost->disable, 0);
		return;
	}

	if (!work_pending(&boost->enable))
		queue_work(boost->boost_wq, &boost->enable);
}

void cpuboost_kick(void)
{
	trigger_event(&kick, dsboost_kick_state);
}

static void cpuboost_input_event(struct input_handle *handle,
		unsigned int type, unsigned int code, int value)
{
	trigger_event(&input, dsboost_input_state);
}

static int cpuboost_input_connect(struct input_handler *handler,
		struct input_dev *dev, const struct input_device_id *id)
{
	struct input_handle *handle;
	int error;

	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "cpufreq";

	error = input_register_handle(handle);
	if (error)
		goto err2;

	error = input_open_device(handle);
	if (error)
		goto err1;

	return 0;
err1:
	input_unregister_handle(handle);
err2:
	kfree(handle);
	return error;
}

static void cpuboost_input_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id cpuboost_ids[] = {
	/* multi-touch touchscreen */
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT |
			INPUT_DEVICE_ID_MATCH_ABSBIT,
		.evbit = { BIT_MASK(EV_ABS) },
		.absbit = { [BIT_WORD(ABS_MT_POSITION_X)] =
			BIT_MASK(ABS_MT_POSITION_X) |
			BIT_MASK(ABS_MT_POSITION_Y) },
	},
	{ }
};

static struct input_handler cpuboost_input_handler = {
	.event          = cpuboost_input_event,
	.connect        = cpuboost_input_connect,
	.disconnect     = cpuboost_input_disconnect,
	.name           = "cpu-boost",
	.id_table       = cpuboost_ids,
};

static int fb_notifier_cb(struct notifier_block *nb, unsigned long action,
			  void *data)
{
	int *blank = ((struct fb_event *) data)->data;
	bool suspend_state = *blank != FB_BLANK_UNBLANK;

	if (action != FB_EARLY_EVENT_BLANK || suspend_state != stored_blank)
		return NOTIFY_OK;

	stored_blank = !suspend_state;

	/* Trigger boosts whenever blank state changes */
	trigger_event(&input, dsboost_input_state);
	trigger_event(&kick, dsboost_kick_state);

	return NOTIFY_OK;
}

static void destroy_boost_workqueues(void)
{
	if (input.boost_wq)
		destroy_workqueue(input.boost_wq);
	if (kick.boost_wq)
		destroy_workqueue(kick.boost_wq);
}

static int init_boost_workqueues(void)
{
	input.boost_wq = alloc_workqueue("input_boost_wq", WQ_HIGHPRI, 1);
	if (!input.boost_wq)
		return -ENOMEM;

	INIT_WORK(&input.enable, trigger_input);	
	INIT_DELAYED_WORK(&input.disable, input_remove);

	kick.boost_wq = alloc_workqueue("kick_boost_wq", WQ_HIGHPRI, 1);
	if (!kick.boost_wq)
		return -ENOMEM;

	INIT_WORK(&kick.enable, trigger_kick);
	INIT_DELAYED_WORK(&kick.disable, kick_remove);

	return 0;
}

static int __init cpu_boost_init(void)
{
	int ret;

	ret = init_boost_workqueues();
	if (ret)
		goto err_wq;

	ret = input_register_handler(&cpuboost_input_handler);
	if (ret)
		goto err_wq;

	fb_notifier.notifier_call = fb_notifier_cb;
	fb_notifier.priority = INT_MAX;
	ret = fb_register_client(&fb_notifier);
	if (ret)
		goto err_input;

	return 0;
err_input:
	input_unregister_handler(&cpuboost_input_handler);
err_wq:
	destroy_boost_workqueues();

	return ret;
}
late_initcall(cpu_boost_init);
