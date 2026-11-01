#include <ghost.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static constexpr g_fs_phys_id G_TEST_HELLO = 0x100000001ULL;
static constexpr g_fs_phys_id G_TEST_BIG = 0x100000002ULL;
static constexpr g_fs_phys_id G_TEST_SCRATCH = 0x100000003ULL;
static constexpr g_fs_phys_id G_TEST_SLOW = 0x100000004ULL;
static constexpr g_fs_phys_id G_TEST_DIE = 0x100000005ULL;
static constexpr g_fs_phys_id G_TEST_EMPTY = 0x100000006ULL;
static constexpr g_fs_phys_id G_TEST_PAUSE = 0x100000007ULL;
static constexpr uint32_t G_TEST_REPORT = 0x46535445;
static constexpr uint32_t G_TEST_STOP = 0x46535354;
static constexpr uint64_t G_TEST_BIG_SIZE = 16431;
static const char testHello[] = "Hello from a task delegate!\n";
static uint8_t testScratch[8192];
static bool serverReady;
static bool pauseStarted;
static unsigned staleReplies;
static g_fs_virt_id testMount;
static g_fs_delegate_transaction_storage* testStorage;
static g_tid coordinator;

static void delegateTestRequire(bool condition, const char* message)
{
	if(!condition)
	{
		klog("fs-delegate-test: FAIL %s", message);
		g_exit(1);
	}
}

static void delegateTestWait(bool* flag, uint64_t timeout, const char* message)
{
	auto start = g_millis();
	while(!__atomic_load_n(flag, __ATOMIC_ACQUIRE) && g_millis() - start < timeout)
		g_sleep(1);
	delegateTestRequire(__atomic_load_n(flag, __ATOMIC_ACQUIRE), message);
}

static uint64_t delegateTestSize(g_fs_phys_id id)
{
	if(id == G_TEST_HELLO) return sizeof(testHello) - 1;
	if(id == G_TEST_BIG) return G_TEST_BIG_SIZE;
	if(id == G_TEST_SCRATCH) return sizeof(testScratch);
	if(id == G_TEST_SLOW || id == G_TEST_DIE || id == G_TEST_PAUSE) return 64;
	return 0;
}

static g_fs_virt_id delegateTestPublish(g_fs_transaction_id transaction, g_fs_virt_id parent,
	const char* name, g_fs_node_type type, g_fs_phys_id physical)
{
	g_fs_virt_id id;
	delegateTestRequire(g_fs_delegate_node_create(transaction, parent, name, type, physical, &id) ==
		G_FS_DELEGATE_SUCCESSFUL, "node publication");
	return id;
}

static void delegateTestHandle(g_fs_transaction_id transaction)
{
	auto storage = testStorage;
	switch(storage->header.type)
	{
	case G_FS_DELEGATE_REQUEST_LOOKUP:
	{
		auto& lookup = storage->data.lookup;
		g_fs_phys_id id = 0;
		const char* canonical = lookup.name;
		if(lookup.parent_phys_fs_id == 0)
		{
			if(!strcmp(lookup.name, "hello.txt") || !strcmp(lookup.name, "HELLO.TXT"))
			{
				id = G_TEST_HELLO;
				canonical = "hello.txt";
			}
			else if(!strcmp(lookup.name, "big.bin")) id = G_TEST_BIG;
			else if(!strcmp(lookup.name, "scratch")) id = G_TEST_SCRATCH;
			else if(!strcmp(lookup.name, "slow")) id = G_TEST_SLOW;
			else if(!strcmp(lookup.name, "die")) id = G_TEST_DIE;
			else if(!strcmp(lookup.name, "empty")) id = G_TEST_EMPTY;
			else if(!strcmp(lookup.name, "pause")) id = G_TEST_PAUSE;
		}
		if(id)
		{
			lookup.result_node_id = delegateTestPublish(transaction, lookup.parent_virt_fs_id, canonical,
				id == G_TEST_EMPTY ? G_FS_NODE_TYPE_FOLDER : G_FS_NODE_TYPE_FILE, id);
			lookup.result_status = G_FS_LOOKUP_SUCCESSFUL;
		}
		else
			lookup.result_status = G_FS_LOOKUP_NOT_FOUND;
		break;
	}
	case G_FS_DELEGATE_REQUEST_READ_DIRECTORY:
	{
		auto& directory = storage->data.directory_refresh;
		if(directory.parent_phys_fs_id == 0)
		{
			delegateTestPublish(transaction, directory.parent_virt_fs_id, "hello.txt", G_FS_NODE_TYPE_FILE, G_TEST_HELLO);
			delegateTestPublish(transaction, directory.parent_virt_fs_id, "big.bin", G_FS_NODE_TYPE_FILE, G_TEST_BIG);
			delegateTestPublish(transaction, directory.parent_virt_fs_id, "empty", G_FS_NODE_TYPE_FOLDER, G_TEST_EMPTY);
		}
		directory.result_status = G_FS_DIRECTORY_REFRESH_SUCCESSFUL;
		break;
	}
	case G_FS_DELEGATE_REQUEST_OPEN:
		storage->data.open.result_status = G_FS_OPEN_SUCCESSFUL;
		break;
	case G_FS_DELEGATE_REQUEST_CLOSE:
		storage->data.close.result_status = G_FS_CLOSE_SUCCESSFUL;
		break;
	case G_FS_DELEGATE_REQUEST_GET_LENGTH:
		storage->data.get_length.result_length = delegateTestSize(storage->data.get_length.phys_fs_id);
		storage->data.get_length.result_status = G_FS_LENGTH_SUCCESSFUL;
		break;
	case G_FS_DELEGATE_REQUEST_READ:
	{
		auto& read = storage->data.read;
		if(read.phys_fs_id == G_TEST_DIE)
			g_exit_task();
		if(read.phys_fs_id == G_TEST_SLOW)
			g_sleep(17000);
		if(read.phys_fs_id == G_TEST_PAUSE)
		{
			__atomic_store_n(&pauseStarted, true, __ATOMIC_RELEASE);
			g_sleep(500);
		}
		auto size = delegateTestSize(read.phys_fs_id);
		auto count = read.offset < size ? size - read.offset : 0;
		if(count > read.length) count = read.length;
		auto buffer = static_cast<uint8_t*>(read.mapped_buffer);
		for(uint64_t i = 0; i < count; ++i)
		{
			if(read.phys_fs_id == G_TEST_HELLO) buffer[i] = testHello[read.offset + i];
			else if(read.phys_fs_id == G_TEST_SCRATCH) buffer[i] = testScratch[read.offset + i];
			else buffer[i] = uint8_t((read.offset + i) % 251);
		}
		read.result_read = count;
		read.result_status = G_FS_READ_SUCCESSFUL;
		break;
	}
	case G_FS_DELEGATE_REQUEST_WRITE:
	{
		auto& write = storage->data.write;
		if(write.phys_fs_id != G_TEST_SCRATCH || write.offset >= sizeof(testScratch))
		{
			write.result_status = G_FS_WRITE_NOT_SUPPORTED;
			break;
		}
		auto count = sizeof(testScratch) - write.offset;
		if(count > write.length) count = write.length;
		memcpy(testScratch + write.offset, write.mapped_buffer, count);
		write.result_write = count;
		write.result_status = G_FS_WRITE_SUCCESSFUL;
		break;
	}
	}
}

static void delegateTestServe()
{
	delegateTestRequire(g_fs_delegate_register("delegate-test", 0, &testMount, &testStorage) ==
		G_FS_DELEGATE_SUCCESSFUL, "register mount");
	__atomic_store_n(&serverReady, true, __ATOMIC_RELEASE);
	for(;;)
	{
		uint8_t incoming[sizeof(g_message_header) + G_MESSAGE_MAXIMUM_MESSAGE_LENGTH];
		delegateTestRequire(g_receive_message(incoming, sizeof(incoming)) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL,
			"receive notification");
		auto message = reinterpret_cast<g_message_header*>(incoming);
		if(message->sender == coordinator && message->length == sizeof(uint32_t))
		{
			uint32_t control;
			memcpy(&control, G_MESSAGE_CONTENT(incoming), sizeof(control));
			if(control == G_TEST_STOP)
			{
				delegateTestRequire(g_fs_delegate_unregister(testMount) == G_FS_DELEGATE_SUCCESSFUL, "unregister");
				return;
			}
		}
		if(message->sender != G_FS_DELEGATE_MESSAGE_SENDER || message->length != sizeof(g_fs_delegate_request))
			continue;
		g_fs_delegate_request request;
		memcpy(&request, G_MESSAGE_CONTENT(incoming), sizeof(request));
		delegateTestRequire(request.version == G_FS_DELEGATE_VERSION && request.mountpoint_id == testMount &&
			request.storage_size == sizeof(*testStorage) && request.transaction == testStorage->header.transaction,
			"notification identity");
		delegateTestHandle(request.transaction);
		auto status = g_fs_delegate_transaction_complete(request.transaction);
		if(status == G_FS_DELEGATE_STALE_TRANSACTION)
			__atomic_add_fetch(&staleReplies, 1, __ATOMIC_RELEASE);
		else
			delegateTestRequire(status == G_FS_DELEGATE_SUCCESSFUL, "complete transaction");
	}
}

static void delegateTestBigFile()
{
	auto fd = g_open("/mount/delegate-test/big.bin");
	delegateTestRequire(fd != G_FD_NONE && g_length(fd) == G_TEST_BIG_SIZE, "large file open/length");
	uint8_t buffer[5000];
	uint64_t offset = 0;
	while(offset < G_TEST_BIG_SIZE)
	{
		auto count = g_read(fd, buffer, sizeof(buffer));
		delegateTestRequire(count > 0 && count <= 4096, "page-sized partial read");
		for(int32_t i = 0; i < count; ++i)
			delegateTestRequire(buffer[i] == uint8_t((offset + i) % 251), "large file data");
		offset += count;
	}
	delegateTestRequire(offset == G_TEST_BIG_SIZE && g_read(fd, buffer, 1) == 0, "EOF");
	delegateTestRequire(g_seek(fd, 4093, G_FS_SEEK_SET) == 4093 && g_read(fd, buffer, 17) == 17,
		"seek and unaligned read");
	for(unsigned i = 0; i < 17; ++i)
		delegateTestRequire(buffer[i] == uint8_t((4093 + i) % 251), "seek data");
	delegateTestRequire(g_close(fd) == G_FS_CLOSE_SUCCESSFUL, "close large file");
}

static int delegateTestClient(g_tid parent, bool cancel)
{
	if(cancel)
	{
		auto fd = g_open("/mount/delegate-test/pause");
		delegateTestRequire(fd != G_FD_NONE, "cancelled client open");
		uint8_t buffer[64];
		g_read(fd, buffer, sizeof(buffer));
		delegateTestRequire(false, "terminated caller resumed userspace");
	}
	g_fs_virt_id mount;
	g_fs_delegate_transaction_storage* storage;
	delegateTestRequire(g_fs_delegate_register("not-privileged", 0, &mount, &storage) == G_FS_DELEGATE_NOT_OWNER,
		"registration privilege");
	auto fd = g_open("/mount/delegate-test/HELLO.TXT");
	delegateTestRequire(fd != G_FD_NONE, "canonical lookup");
	char hello[64]{};
	delegateTestRequire(g_read(fd, hello, sizeof(hello)) == sizeof(testHello) - 1 && !strcmp(hello, testHello),
		"hello file read");
	delegateTestRequire(g_close(fd) == G_FS_CLOSE_SUCCESSFUL, "hello close");
	delegateTestRequire(g_open("/mount/delegate-test/missing") == G_FD_NONE, "missing file");
	auto directory = g_open_directory("/mount/delegate-test");
	delegateTestRequire(directory != nullptr, "directory refresh");
	unsigned entries = 0;
	while(g_read_directory(directory)) ++entries;
	delegateTestRequire(entries >= 3, "directory entries");
	g_close_directory(directory);
	directory = g_open_directory("/mount/delegate-test/empty");
	delegateTestRequire(directory != nullptr, "empty directory refresh");
	g_fs_read_directory_status status;
	delegateTestRequire(!g_read_directory_s(directory, &status) && status == G_FS_READ_DIRECTORY_EOD,
		"empty directory EOF");
	g_close_directory(directory);
	delegateTestBigFile();
	fd = g_open_f("/mount/delegate-test/scratch", G_FILE_FLAG_MODE_READ | G_FILE_FLAG_MODE_WRITE);
	delegateTestRequire(fd != G_FD_NONE && g_seek(fd, 13, G_FS_SEEK_SET) == 13, "scratch open/seek");
	const char text[] = "shared write bytes";
	delegateTestRequire(g_write(fd, text, sizeof(text)) == sizeof(text), "write");
	delegateTestRequire(g_seek(fd, 13, G_FS_SEEK_SET) == 13 && g_read(fd, hello, sizeof(text)) == sizeof(text) &&
		!memcmp(text, hello, sizeof(text)), "write readback");
	delegateTestRequire(g_close(fd) == G_FS_CLOSE_SUCCESSFUL, "scratch close");
	uint32_t report = G_TEST_REPORT;
	delegateTestRequire(g_send_message(parent, &report, sizeof(report)) == G_MESSAGE_SEND_STATUS_SUCCESSFUL,
		"child report");
	return 0;
}

static void delegateTestReader()
{
	for(unsigned i = 0; i < 6; ++i)
		delegateTestBigFile();
}

static g_tid delegateTestStartServer()
{
	__atomic_store_n(&serverReady, false, __ATOMIC_RELEASE);
	auto server = g_create_task(reinterpret_cast<void*>(delegateTestServe));
	delegateTestRequire(server != G_TID_NONE, "serving thread creation");
	delegateTestWait(&serverReady, 5000, "serving thread startup");
	return server;
}

int main(int argc, char** argv)
{
	if(argc >= 3)
		return delegateTestClient(g_tid(strtol(argv[2], nullptr, 10)), !strcmp(argv[1], "cancel"));
	coordinator = g_get_tid();
	klog("fs-delegate-test: START");
	auto server = delegateTestStartServer();
	char args[64];
	snprintf(args, sizeof(args), "client%c%i", G_CLIARGS_SEPARATOR, coordinator);
	g_pid child;
	delegateTestRequire(g_spawn_p("/applications/ahcidriver.bin", args, "/", G_SECURITY_LEVEL_APPLICATION,
		&child) == G_SPAWN_STATUS_SUCCESSFUL, "client spawn");
	uint8_t incoming[sizeof(g_message_header) + sizeof(uint32_t)];
	delegateTestRequire(g_receive_message(incoming, sizeof(incoming)) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL,
		"client completion");
	uint32_t report;
	memcpy(&report, G_MESSAGE_CONTENT(incoming), sizeof(report));
	delegateTestRequire(report == G_TEST_REPORT && reinterpret_cast<g_message_header*>(incoming)->sender == child,
		"client report identity");
	g_join(child);
	klog("fs-delegate-test: application lookup/directory/read/write PASS");
	g_tid readers[4];
	for(auto& reader : readers)
	{
		reader = g_create_task(reinterpret_cast<void*>(delegateTestReader));
		delegateTestRequire(reader != G_TID_NONE, "reader creation");
	}
	for(auto reader : readers) g_join(reader);
	klog("fs-delegate-test: concurrent clients PASS");
	__atomic_store_n(&pauseStarted, false, __ATOMIC_RELEASE);
	snprintf(args, sizeof(args), "cancel%c%i", G_CLIARGS_SEPARATOR, coordinator);
	delegateTestRequire(g_spawn_p("/applications/ahcidriver.bin", args, "/", G_SECURITY_LEVEL_APPLICATION,
		&child) == G_SPAWN_STATUS_SUCCESSFUL, "cancel client spawn");
	delegateTestWait(&pauseStarted, 5000, "cancel client request");
	delegateTestRequire(g_kill(child) == G_KILL_STATUS_SUCCESSFUL, "cancel client termination");
	g_join(child);
	g_sleep(600);
	delegateTestBigFile();
	klog("fs-delegate-test: caller termination PASS");
	auto fd = g_open("/mount/delegate-test/slow");
	delegateTestRequire(fd != G_FD_NONE, "slow file open");
	uint8_t reused[64]{};
	auto previous = __atomic_load_n(&staleReplies, __ATOMIC_ACQUIRE);
	auto start = g_millis();
	delegateTestRequire(g_read(fd, reused, sizeof(reused)) == -1 && g_millis() - start >= 14990,
		"request deadline");
	memset(reused, 0xa5, sizeof(reused));
	start = g_millis();
	while(__atomic_load_n(&staleReplies, __ATOMIC_ACQUIRE) == previous && g_millis() - start < 5000) g_sleep(1);
	delegateTestRequire(__atomic_load_n(&staleReplies, __ATOMIC_ACQUIRE) > previous, "late completion acknowledgement");
	for(auto value : reused) delegateTestRequire(value == 0xa5, "late write modified reused caller memory");
	g_close(fd);
	klog("fs-delegate-test: timeout/late write isolation PASS");
	auto old = g_open("/mount/delegate-test/big.bin");
	uint32_t stop = G_TEST_STOP;
	delegateTestRequire(g_send_message(server, &stop, sizeof(stop)) == G_MESSAGE_SEND_STATUS_SUCCESSFUL, "stop server");
	g_join(server);
	delegateTestRequire(g_open("/mount/delegate-test/big.bin") == G_FD_NONE && g_read(old, reused, 1) == -1,
		"unregistered mount access");
	server = delegateTestStartServer();
	delegateTestRequire(g_read(old, reused, 1) == -1, "old descriptor retargeted after remount");
	g_close(old);
	fd = g_open("/mount/delegate-test/die");
	delegateTestRequire(fd != G_FD_NONE && g_read(fd, reused, 1) == -1, "serving task death");
	g_join(server);
	g_close(fd);
	delegateTestRequire(g_open("/mount/delegate-test/big.bin") == G_FD_NONE, "dead mount still visible");
	klog("fs-delegate-test: PASS mount lifecycle and serving-task cleanup");
	return 0;
}
