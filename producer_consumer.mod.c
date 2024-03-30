#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

#ifdef CONFIG_UNWINDER_ORC
#include <asm/orc_header.h>
ORC_HEADER;
#endif

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x6df1aaf1, "kernel_sigaction" },
	{ 0x6626afca, "down" },
	{ 0xb3f7646e, "kthread_should_stop" },
	{ 0xaa7d5f63, "init_task" },
	{ 0xcc55bbc1, "pcpu_hot" },
	{ 0xcf2a6966, "up" },
	{ 0x122c3a7e, "_printk" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0x87a21cb3, "__ubsan_handle_out_of_bounds" },
	{ 0x688c2f28, "kthread_stop" },
	{ 0xb43f9365, "ktime_get" },
	{ 0x349cba85, "strchr" },
	{ 0x4a22c67a, "kthread_create_on_node" },
	{ 0xa48bdf06, "wake_up_process" },
	{ 0xdf3052d7, "param_ops_int" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x4c9adaff, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "61457A5B98FD8383BC093D3");
