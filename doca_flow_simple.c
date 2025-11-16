/*
 * Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
 *
 * This software product is a proprietary product of NVIDIA CORPORATION &
 * AFFILIATES (the "Company") and all right, title, and interest in and to the
 * software product, including all associated intellectual property rights, are
 * and shall remain exclusively with the Company.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <doca_log.h>
#include <doca_flow.h>
#include <doca_dpdk.h>

DOCA_LOG_REGISTER(FLOW_SIMPLE);

#define NB_PORTS 2
#define MAX_PORT_STR_LEN 128

struct doca_flow_port *ports[NB_PORTS];

/*
 * Initialize DOCA Flow library
 *
 * @nb_queues [in]: number of queues the sample will use
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
static doca_error_t
init_doca_flow(int nb_queues)
{
	struct doca_flow_cfg flow_cfg = {0};

	flow_cfg.queues = nb_queues;
	flow_cfg.mode_args = "vnf,hws";
	flow_cfg.resource.nb_counters = 1024;

	return doca_flow_init(&flow_cfg);
}

/*
 * Initialize DOCA Flow ports
 *
 * @nb_ports [in]: number of ports to create
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
static doca_error_t
init_doca_flow_ports(int nb_ports)
{
	int portid;
	char port_id_str[MAX_PORT_STR_LEN];
	struct doca_flow_port_cfg port_cfg = {0};

	for (portid = 0; portid < nb_ports; portid++) {
		/* Create doca flow port */
		port_cfg.port_id = portid;
		port_cfg.type = DOCA_FLOW_PORT_DPDK_BY_ID;
		snprintf(port_id_str, MAX_PORT_STR_LEN, "%d", port_cfg.port_id);
		port_cfg.devargs = port_id_str;

		if (doca_flow_port_start(&port_cfg, &ports[portid]) != DOCA_SUCCESS) {
			DOCA_LOG_ERR("Failed to start port %d", portid);
			return DOCA_ERROR_INITIALIZATION;
		}
	}

	return DOCA_SUCCESS;
}

/*
 * Create a simple control pipe that forwards all traffic to port
 *
 * @port [in]: port to create the pipe on
 * @port_id [in]: destination port ID
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
static doca_error_t
create_control_pipe(struct doca_flow_port *port, uint16_t port_id)
{
	struct doca_flow_pipe_cfg pipe_cfg = {0};
	struct doca_flow_pipe *pipe;
	struct doca_flow_match match = {0};
	struct doca_flow_fwd fwd = {0};
	struct doca_flow_pipe_entry *entry;
	doca_error_t result;

	pipe_cfg.attr.name = "CONTROL_PIPE";
	pipe_cfg.attr.type = DOCA_FLOW_PIPE_CONTROL;
	pipe_cfg.attr.is_root = true;
	pipe_cfg.port = port;

	result = doca_flow_pipe_create(&pipe_cfg, &fwd, NULL, &pipe);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to create control pipe: %s", doca_error_get_descr(result));
		return result;
	}

	/* Match all traffic and forward to the other port */
	fwd.type = DOCA_FLOW_FWD_PORT;
	fwd.port_id = port_id;

	result = doca_flow_pipe_control_add_entry(0, 0, pipe, &match, NULL, NULL, NULL, NULL, NULL, NULL, &fwd, NULL, &entry);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to add control pipe entry: %s", doca_error_get_descr(result));
		return result;
	}

	result = doca_flow_entries_process(port, 0, 0, 0);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to process entries: %s", doca_error_get_descr(result));
		return result;
	}

	DOCA_LOG_INFO("Created control pipe on port %d forwarding to port %d", 
		      doca_flow_port_switch_get(port) ? doca_flow_port_switch_get(port)->port_id : 0, 
		      port_id);

	return DOCA_SUCCESS;
}

/*
 * Main function
 *
 * @argc [in]: command line arguments count
 * @argv [in]: command line arguments
 * @return: EXIT_SUCCESS on success and EXIT_FAILURE otherwise
 */
int
main(int argc, char **argv)
{
	doca_error_t result;
	int nb_ports = NB_PORTS;
	struct doca_log_backend *sdk_log;
	int ret;

	/* Initialize DPDK */
	ret = rte_eal_init(argc, argv);
	if (ret < 0) {
		DOCA_LOG_ERR("DPDK initialization failed: %d", ret);
		return EXIT_FAILURE;
	}

	/* Initialize DOCA logging */
	result = doca_log_backend_create_standard();
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to create log backend");
		return EXIT_FAILURE;
	}

	result = doca_log_backend_create_with_file_sdk(stderr, &sdk_log);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to create SDK log backend");
		return EXIT_FAILURE;
	}

	result = doca_log_backend_set_sdk_level(sdk_log, DOCA_LOG_LEVEL_WARNING);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to set SDK log level");
		return EXIT_FAILURE;
	}

	DOCA_LOG_INFO("Starting DOCA Flow Simple Example");

	/* Initialize DOCA Flow */
	result = init_doca_flow(nb_ports);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to initialize DOCA Flow: %s", doca_error_get_descr(result));
		return EXIT_FAILURE;
	}

	/* Start DOCA Flow ports */
	result = init_doca_flow_ports(nb_ports);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to initialize DOCA Flow ports: %s", doca_error_get_descr(result));
		doca_flow_destroy();
		return EXIT_FAILURE;
	}

	/* Create control pipes for bidirectional forwarding */
	result = create_control_pipe(ports[0], 1);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to create control pipe for port 0");
		goto cleanup;
	}

	result = create_control_pipe(ports[1], 0);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to create control pipe for port 1");
		goto cleanup;
	}

	DOCA_LOG_INFO("DOCA Flow pipes created successfully");
	DOCA_LOG_INFO("Application running - Press Ctrl+C to exit");

	/* Keep application running */
	while (1)
		sleep(1);

cleanup:
	/* Cleanup */
	for (int i = 0; i < nb_ports; i++) {
		if (ports[i] != NULL)
			doca_flow_port_stop(ports[i]);
	}

	doca_flow_destroy();

	return result == DOCA_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE;
}
