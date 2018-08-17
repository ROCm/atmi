/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/
#ifndef HSA_RUNTIME_INC_HSA_H_
#define HSA_RUNTIME_INC_HSA_H_

/* Unsigned.  */
typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long int uint64_t;

typedef signed char int8_t;
typedef short int int16_t;
typedef int int32_t;
typedef long int int64_t;

/*#include <stddef.h> *//* size_t */
/*#include <stdint.h> *//* uintXX_t */
/*#ifndef __cplusplus*/
/*#include <stdbool.h>*/
/*#endif *//* __cplusplus */

// Placeholder for calling convention and import macros
#define HSA_CALL
#undef HSA_API
#define HSA_API HSA_CALL

// Detect and set large model builds.
/*#undef HSA_LARGE_MODEL*/
/*#if defined(__LP64__) || defined(_M_X64)*/
/*#define HSA_LARGE_MODEL*/
/*#endif*/
#define HSA_LARGE_MODEL

// Try to detect CPU endianness
/*#if !defined(LITTLEENDIAN_CPU) && !defined(BIGENDIAN_CPU)*/
/*#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || \*/
/*defined(_M_X64)*/
#define LITTLEENDIAN_CPU
/*#endif*/
/*#endif*/

/*#undef HSA_LITTLE_ENDIAN*/
/*#if defined(LITTLEENDIAN_CPU)*/
#define HSA_LITTLE_ENDIAN
/*#elif defined(BIGENDIAN_CPU)*/
/*#else*/
/*#error "BIGENDIAN_CPU or LITTLEENDIAN_CPU must be defined"*/
/*#endif*/

#define OBSIDIAN_RUNTIME

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** \defgroup status Runtime Notifications
 *  @{
 */

/**
 * @brief Status codes.
 */
typedef enum {
  /**
   * The function has been executed successfully.
   */
  HSA_STATUS_SUCCESS = 0x0,
  /**
   * A traversal over a list of elements has been interrupted by the
   * application before completing.
   */
  HSA_STATUS_INFO_BREAK = 0x1,
  /**
   * A generic error has occurred.
   */
  HSA_STATUS_ERROR = 0x1000,
  /**
   * One of the actual arguments does not meet a precondition stated in the
   * documentation of the corresponding formal argument.
   */
  HSA_STATUS_ERROR_INVALID_ARGUMENT = 0x1001,
  /**
   * The requested queue creation is not valid.
   */
  HSA_STATUS_ERROR_INVALID_QUEUE_CREATION = 0x1002,
  /**
   * The requested allocation is not valid.
   */
  HSA_STATUS_ERROR_INVALID_ALLOCATION = 0x1003,
  /**
   * The agent is invalid.
   */
  HSA_STATUS_ERROR_INVALID_AGENT = 0x1004,
  /**
   * The memory region is invalid.
   */
  HSA_STATUS_ERROR_INVALID_REGION = 0x1005,
  /**
   * The signal is invalid.
   */
  HSA_STATUS_ERROR_INVALID_SIGNAL = 0x1006,
  /**
   * The queue is invalid.
   */
  HSA_STATUS_ERROR_INVALID_QUEUE = 0x1007,
  /**
   * The HSA runtime failed to allocate the necessary resources. This error
   * may also occur when the HSA runtime needs to spawn threads or create
   * internal OS-specific events.
   */
  HSA_STATUS_ERROR_OUT_OF_RESOURCES = 0x1008,
  /**
   * The AQL packet is malformed.
   */
  HSA_STATUS_ERROR_INVALID_PACKET_FORMAT = 0x1009,
  /**
   * An error has been detected while releasing a resource.
   */
  HSA_STATUS_ERROR_RESOURCE_FREE = 0x100A,
  /**
   * An API other than ::hsa_init has been invoked while the reference count
   * of the HSA runtime is 0.
   */
  HSA_STATUS_ERROR_NOT_INITIALIZED = 0x100B,
  /**
   * The maximum reference count for the object has been reached.
   */
  HSA_STATUS_ERROR_REFCOUNT_OVERFLOW = 0x100C,
  /**
   * The arguments passed to a functions are not compatible.
   */
  HSA_STATUS_ERROR_INCOMPATIBLE_ARGUMENTS = 0x100D,
  /**
   * The index is invalid.
   */
  HSA_STATUS_ERROR_INVALID_INDEX = 0x100E,
  /**
   * The instruction set architecture is invalid.
   */
  HSA_STATUS_ERROR_INVALID_ISA = 0x100F,
  /**
   * The instruction set architecture name is invalid.
   */
  HSA_STATUS_ERROR_INVALID_ISA_NAME = 0x1017,
  /**
   * The code object is invalid.
   */
  HSA_STATUS_ERROR_INVALID_CODE_OBJECT = 0x1010,
  /**
   * The executable is invalid.
   */
  HSA_STATUS_ERROR_INVALID_EXECUTABLE = 0x1011,
  /**
   * The executable is frozen.
   */
  HSA_STATUS_ERROR_FROZEN_EXECUTABLE = 0x1012,
  /**
   * There is no symbol with the given name.
   */
  HSA_STATUS_ERROR_INVALID_SYMBOL_NAME = 0x1013,
  /**
   * The variable is already defined.
   */
  HSA_STATUS_ERROR_VARIABLE_ALREADY_DEFINED = 0x1014,
  /**
   * The variable is undefined.
   */
  HSA_STATUS_ERROR_VARIABLE_UNDEFINED = 0x1015,
  /**
   * An HSAIL operation resulted on a hardware exception.
   */
  HSA_STATUS_ERROR_EXCEPTION = 0x1016
} hsa_status_t;

/**
 * @brief Query additional information about a status code.
 *
 * @param[in] status Status code.
 *
 * @param[out] status_string A NUL-terminated string that describes the error
 * status.
 *
 * @retval ::HSA_STATUS_SUCCESS The function has been executed successfully.
 *
 * @retval ::HSA_STATUS_ERROR_NOT_INITIALIZED The HSA runtime has not been
 * initialized.
 *
 * @retval ::HSA_STATUS_ERROR_INVALID_ARGUMENT @p status is an invalid
 * status code, or @p status_string is NULL.
 */
hsa_status_t HSA_API
    hsa_status_string(hsa_status_t status, const char **status_string);

/** @} */

/** \defgroup common Common Definitions
 *  @{
 */

/**
 * @brief Three-dimensional coordinate.
 */
typedef struct hsa_dim3_s {
  /**
   * X dimension.
   */
  uint32_t x;

  /**
   * Y dimension.
   */
  uint32_t y;

  /**
   * Z dimension.
   */
  uint32_t z;
} hsa_dim3_t;

/**
 * @brief Access permissions.
 */
typedef enum {
  /**
   * Read-only access.
   */
  HSA_ACCESS_PERMISSION_RO = 1,
  /**
   * Write-only access.
   */
  HSA_ACCESS_PERMISSION_WO = 2,
  /**
   * Read and write access.
   */
  HSA_ACCESS_PERMISSION_RW = 3
} hsa_access_permission_t;

/** @} **/

/** \defgroup initshutdown Initialization and Shut Down
 *  @{
 */

/**
 * @brief Initialize the HSA runtime.
 *
 * @details Initializes the HSA runtime if it is not already initialized, and
 * increases the reference counter associated with the HSA runtime for the
 * current process. Invocation of any HSA function other than ::hsa_init results
 * in undefined behavior if the current HSA runtime reference counter is less
 * than one.
 *
 * @retval ::HSA_STATUS_SUCCESS The function has been executed successfully.
 *
 * @retval ::HSA_STATUS_ERROR_OUT_OF_RESOURCES There is failure to allocate
 * the resources required by the implementation.
 *
 * @retval ::HSA_STATUS_ERROR_REFCOUNT_OVERFLOW The HSA runtime reference
 * count reaches INT32_MAX.
 */
hsa_status_t HSA_API hsa_init();

/**
 * @brief Shut down the HSA runtime.
 *
 * @details Decreases the reference count of the HSA runtime instance. When the
 * reference count reaches 0, the HSA runtime is no longer considered valid
 * but the application might call ::hsa_init to initialize the HSA runtime
 * again.
 *
 * Once the reference count of the HSA runtime reaches 0, all the resources
 * associated with it (queues, signals, agent information, etc.) are
 * considered invalid and any attempt to reference them in subsequent API calls
 * results in undefined behavior. When the reference count reaches 0, the HSA
 * runtime may release resources associated with it.
 *
 * @retval ::HSA_STATUS_SUCCESS The function has been executed successfully.
 *
 * @retval ::HSA_STATUS_ERROR_NOT_INITIALIZED The HSA runtime has not been
 * initialized.
 *
 */
hsa_status_t HSA_API hsa_shut_down();

/** @} **/

/** \defgroup agentinfo System and Agent Information
 *  @{
 */

/**
 * @brief Endianness. A convention used to interpret the bytes making up a data
 * word.
 */
typedef enum {
  /**
   * The least significant byte is stored in the smallest address.
   */
  HSA_ENDIANNESS_LITTLE = 0,
  /**
   * The most significant byte is stored in the smallest address.
   */
  HSA_ENDIANNESS_BIG = 1
} hsa_endianness_t;

/**
 * @brief Machine model. A machine model determines the size of certain data
 * types in HSA runtime and an agent.
 */
typedef enum {
  /**
   * Small machine model. Addresses use 32 bits.
   */
  HSA_MACHINE_MODEL_SMALL = 0,
  /**
   * Large machine model. Addresses use 64 bits.
   */
  HSA_MACHINE_MODEL_LARGE = 1
} hsa_machine_model_t;

/**
 * @brief Profile. A profile indicates a particular level of feature
 * support. For example, in the base profile the application must use the HSA
 * runtime allocator to reserve Shared Virtual Memory, while in the full profile
 * any host pointer can be shared across all the agents.
 */
typedef enum {
  /**
   * Base profile.
   */
  HSA_PROFILE_BASE = 0,
  /**
   * Full profile.
   */
  HSA_PROFILE_FULL = 1
} hsa_profile_t;

/**
 * @brief System attributes.
 */
typedef enum {
  /**
   * Major version of the HSA runtime specification supported by the
   * implementation. The type of this attribute is uint16_t.
   */
  HSA_SYSTEM_INFO_VERSION_MAJOR = 0,
  /**
   * Minor version of the HSA runtime specification supported by the
   * implementation. The type of this attribute is uint16_t.
   */
  HSA_SYSTEM_INFO_VERSION_MINOR = 1,
  /**
   * Current timestamp. The value of this attribute monotonically increases at a
   * constant rate. The type of this attribute is uint64_t.
   */
  HSA_SYSTEM_INFO_TIMESTAMP = 2,
  /**
   * Timestamp value increase rate, in Hz. The timestamp (clock) frequency is
   * in the range 1-400MHz. The type of this attribute is uint64_t.
   */
  HSA_SYSTEM_INFO_TIMESTAMP_FREQUENCY = 3,
  /**
   * Maximum duration of a signal wait operation. Expressed as a count based on
   * the timestamp frequency. The type of this attribute is uint64_t.
   */
  HSA_SYSTEM_INFO_SIGNAL_MAX_WAIT = 4,
  /**
   * Endianness of the system. The type of this attribute us ::hsa_endianness_t.
   */
  HSA_SYSTEM_INFO_ENDIANNESS = 5,
  /**
   * Machine model supported by the HSA runtime. The type of this attribute is
   * ::hsa_machine_model_t.
   */
  HSA_SYSTEM_INFO_MACHINE_MODEL = 6,
  /**
   * Bit-mask indicating which extensions are supported by the
   * implementation. An extension with an ID of @p i is supported if the bit at
   * position @p i is set. The type of this attribute is uint8_t[128].
   */
  HSA_SYSTEM_INFO_EXTENSIONS = 7
} hsa_system_info_t;

/**
 * @brief Get the current value of a system attribute.
 *
 * @param[in] attribute Attribute to query.
 *
 * @param[out] value Pointer to an application-allocated buffer where to store
 * the value of the attribute. If the buffer passed by the application is not
 * large enough to hold the value of @p attribute, the behavior is undefined.
 *
 * @retval ::HSA_STATUS_SUCCESS The function has been executed successfully.
 *
 * @retval ::HSA_STATUS_ERROR_NOT_INITIALIZED The HSA runtime has not been
 * initialized.
 *
 * @retval ::HSA_STATUS_ERROR_INVALID_ARGUMENT @p attribute is an invalid
 * system attribute, or @p value is NULL.
 */
hsa_status_t HSA_API
    hsa_system_get_info(hsa_system_info_t attribute, void *value);

/**
 * @brief HSA extensions.
 */
typedef enum {
  /**
   * Finalizer extension.
   */
  HSA_EXTENSION_FINALIZER = 0,
  /**
   * Images extension.
   */
  HSA_EXTENSION_IMAGES = 1,
  HSA_EXTENSION_AMD_PROFILER = 2
} hsa_extension_t;

/**
 * @brief Query if a given version of an extension is supported by the HSA
 * implementation.
 *
 * @param[in] extension Extension identifier.
 *
 * @param[in] version_major Major version number.
 *
 * @param[in] version_minor Minor version number.
 *
 * @param[out] result Pointer to a memory location where the HSA runtime stores
 * the result of the check. The result is true if the specified version of the
 * extension is supported, and false otherwise.
 *
 * @retval ::HSA_STATUS_SUCCESS The function has been executed successfully.
 *
 * @retval ::HSA_STATUS_ERROR_NOT_INITIALIZED The HSA runtime has not been
 * initialized.
 *
 * @retval ::HSA_STATUS_ERROR_INVALID_ARGUMENT @p extension is not a valid
 * extension, or @p result is NULL.
 */
hsa_status_t HSA_API
    hsa_system_extension_supported(uint16_t extension, uint16_t version_major,
                                   uint16_t version_minor, bool *result);

/**
 * @brief Retrieve the function pointers corresponding to a given version of an
 * extension. Portable applications are expected to invoke the extension API
 * using the returned function pointers
 *
 * @details The application is responsible for verifying that the given version
 * of the extension is supported by the HSA implementation (see
 * ::hsa_system_extension_supported). If the given combination of extension,
 * major version, and minor version is not supported by the implementation, the
 * behavior is undefined.
 *
 * @param[in] extension Extension identifier.
 *
 * @param[in] version_major Major version number for which to retrieve the
 * function pointer table.
 *
 * @param[in] version_minor Minor version number for which to retrieve the
 * function pointer table.
 *
 * @param[out] table Pointer to an application-allocated function pointer table
 * that is populated by the HSA runtime. Must not be NULL. The memory associated
 * with table can be reused or freed after the function returns.
 *
 * @retval ::HSA_STATUS_SUCCESS The function has been executed successfully.
 *
 * @retval ::HSA_STATUS_ERROR_NOT_INITIALIZED The HSA runtime has not been
 * initialized.
 *
 * @retval ::HSA_STATUS_ERROR_INVALID_ARGUMENT @p extension is not a valid
 * extension, or @p table is NULL.
 */
hsa_status_t HSA_API
    hsa_system_get_extension_table(uint16_t extension, uint16_t version_major,
                                   uint16_t version_minor, void *table);

/**
 * @brief Opaque handle representing an agent, a device that participates in
 * the HSA memory model. An agent can submit AQL packets for execution, and
 * may also accept AQL packets for execution (agent dispatch packets or kernel
 * dispatch packets launching HSAIL-derived binaries).
 */
typedef struct hsa_agent_s {
  /**
   * Opaque handle.
   */
  uint64_t handle;
} hsa_agent_t;

/**
 * @brief Agent features.
 */
typedef enum {
  /**
   * The agent supports AQL packets of kernel dispatch type. If this
   * feature is enabled, the agent is also a kernel agent.
   */
  HSA_AGENT_FEATURE_KERNEL_DISPATCH = 1,
  /**
   * The agent supports AQL packets of agent dispatch type.
   */
  HSA_AGENT_FEATURE_AGENT_DISPATCH = 2
} hsa_agent_feature_t;

/**
 * @brief Hardware device type.
 */
typedef enum {
  /**
   * CPU device.
   */
  HSA_DEVICE_TYPE_CPU = 0,
  /**
   * GPU device.
   */
  HSA_DEVICE_TYPE_GPU = 1,
  /**
   * DSP device.
   */
  HSA_DEVICE_TYPE_DSP = 2
} hsa_device_type_t;

/**
 * @brief Default floating-point rounding mode.
 */
typedef enum {
  /**
   * Use a default floating-point rounding mode specified elsewhere.
   */
  HSA_DEFAULT_FLOAT_ROUNDING_MODE_DEFAULT = 0,
  /**
   * Operations that specify the default floating-point mode are rounded to zero
   * by default.
   */
  HSA_DEFAULT_FLOAT_ROUNDING_MODE_ZERO = 1,
  /**
   * Operations that specify the default floating-point mode are rounded to the
   * nearest representable number and that ties should be broken by selecting
   * the value with an even least significant bit.
   */
  HSA_DEFAULT_FLOAT_ROUNDING_MODE_NEAR = 2
} hsa_default_float_rounding_mode_t;

/** @} */

/** \defgroup signals Signals
 *  @{
 */

/**
 * @brief Signal handle.
 */

/*typedef struct hsa_signal_s {*/
/*  *//***/
/** Opaque handle. The value 0 is reserved.*/
/*   */
/*uint64_t handle;*/
/*} hsa_signal_t;*/

typedef uint64_t hsa_signal_t;

/**
 * @brief Signal value. The value occupies 32 bits in small machine mode, and 64
 * bits in large machine mode.
 */
#ifdef HSA_LARGE_MODEL
typedef int64_t hsa_signal_value_t;
#else
typedef int32_t hsa_signal_value_t;
#endif

/**
 * @brief Create a signal.
 *
 * @param[in] initial_value Initial value of the signal.
 *
 * @param[in] num_consumers Size of @p consumers. A value of 0 indicates that
 * any agent might wait on the signal.
 *
 * @param[in] consumers List of agents that might consume (wait on) the
 * signal. If @p num_consumers is 0, this argument is ignored; otherwise, the
 * HSA runtime might use the list to optimize the handling of the signal
 * object. If an agent not listed in @p consumers waits on the returned
 * signal, the behavior is undefined. The memory associated with @p consumers
 * can be reused or freed after the function returns.
 *
 * @param[out] signal Pointer to a memory location where the HSA runtime will
 * store the newly created signal handle.
 *
 * @retval ::HSA_STATUS_SUCCESS The function has been executed successfully.
 *
 * @retval ::HSA_STATUS_ERROR_NOT_INITIALIZED The HSA runtime has not been
 * initialized.
 *
 * @retval ::HSA_STATUS_ERROR_OUT_OF_RESOURCES There is failure to allocate the
 * resources required by the implementation.
 *
 * @retval ::HSA_STATUS_ERROR_INVALID_ARGUMENT @p signal is NULL, @p
 * num_consumers is greater than 0 but @p consumers is NULL, or @p consumers
 * contains duplicates.
 */
hsa_status_t HSA_API
    hsa_signal_create(hsa_signal_value_t initial_value, uint32_t num_consumers,
                      const hsa_agent_t *consumers, hsa_signal_t *signal);

/**
 * @brief Destroy a signal previous created by ::hsa_signal_create.
 *
 * @param[in] signal Signal.
 *
 * @retval ::HSA_STATUS_SUCCESS The function has been executed successfully.
 *
 * @retval ::HSA_STATUS_ERROR_NOT_INITIALIZED The HSA runtime has not been
 * initialized.
 *
 * @retval ::HSA_STATUS_ERROR_INVALID_SIGNAL @p signal is invalid.
 *
 * @retval ::HSA_STATUS_ERROR_INVALID_ARGUMENT The handle in @p signal is 0.
 */
hsa_status_t HSA_API hsa_signal_destroy(hsa_signal_t signal);

/**
 * @brief Atomically read the current value of a signal.
 *
 * @param[in] signal Signal.
 *
 * @return Value of the signal.
 */
hsa_signal_value_t HSA_API hsa_signal_load_acquire(hsa_signal_t signal);

/**
 * @copydoc hsa_signal_load_acquire
 */
hsa_signal_value_t HSA_API hsa_signal_load_relaxed(hsa_signal_t signal);

/**
 * @brief Atomically set the value of a signal.
 *
 * @details If the value of the signal is changed, all the agents waiting
 * on @p signal for which @p value satisfies their wait condition are awakened.
 *
 * @param[in] signal Signal.
 *
 * @param[in] value New signal value.
 */
void HSA_API
    hsa_signal_store_relaxed(hsa_signal_t signal, hsa_signal_value_t value);

/**
 * @copydoc hsa_signal_store_relaxed
 */
void HSA_API
    hsa_signal_store_release(hsa_signal_t signal, hsa_signal_value_t value);

/**
 * @brief Atomically set the value of a signal and return its previous value.
 *
 * @details If the value of the signal is changed, all the agents waiting
 * on @p signal for which @p value satisfies their wait condition are awakened.
 *
 * @param[in] signal Signal. If @p signal is a queue doorbell signal, the
 * behavior is undefined.
 *
 * @param[in] value New value.
 *
 * @return Value of the signal prior to the exchange.
 *
 */
hsa_signal_value_t HSA_API
    hsa_signal_exchange_acq_rel(hsa_signal_t signal, hsa_signal_value_t value);

/**
 * @copydoc hsa_signal_exchange_acq_rel
 */
hsa_signal_value_t HSA_API
    hsa_signal_exchange_acquire(hsa_signal_t signal, hsa_signal_value_t value);

/**
 * @copydoc hsa_signal_exchange_acq_rel
 */
hsa_signal_value_t HSA_API
    hsa_signal_exchange_relaxed(hsa_signal_t signal, hsa_signal_value_t value);

/**
 * @copydoc hsa_signal_exchange_acq_rel
 */
hsa_signal_value_t HSA_API
    hsa_signal_exchange_release(hsa_signal_t signal, hsa_signal_value_t value);

/**
 * @brief Atomically set the value of a signal if the observed value is equal to
 * the expected value. The observed value is returned regardless of whether the
 * replacement was done.
 *
 * @details If the value of the signal is changed, all the agents waiting
 * on @p signal for which @p value satisfies their wait condition are awakened.
 *
 * @param[in] signal Signal. If @p signal is a queue
 * doorbell signal, the behavior is undefined.
 *
 * @param[in] expected Value to compare with.
 *
 * @param[in] value New value.
 *
 * @return Observed value of the signal.
 *
 */
hsa_signal_value_t HSA_API hsa_signal_cas_acq_rel(hsa_signal_t signal,
                                                  hsa_signal_value_t expected,
                                                  hsa_signal_value_t value);

/**
 * @copydoc hsa_signal_cas_acq_rel
 */
hsa_signal_value_t HSA_API hsa_signal_cas_acquire(hsa_signal_t signal,
                                                  hsa_signal_value_t expected,
                                                  hsa_signal_value_t value);

/**
 * @copydoc hsa_signal_cas_acq_rel
 */
hsa_signal_value_t HSA_API hsa_signal_cas_relaxed(hsa_signal_t signal,
                                                  hsa_signal_value_t expected,
                                                  hsa_signal_value_t value);

/**
 * @copydoc hsa_signal_cas_acq_rel
 */
hsa_signal_value_t HSA_API hsa_signal_cas_release(hsa_signal_t signal,
                                                  hsa_signal_value_t expected,
                                                  hsa_signal_value_t value);

/**
 * @brief Atomically increment the value of a signal by a given amount.
 *
 * @details If the value of the signal is changed, all the agents waiting on
 * @p signal for which @p value satisfies their wait condition are awakened.
 *
 * @param[in] signal Signal. If @p signal is a queue doorbell signal, the
 * behavior is undefined.
 *
 * @param[in] value Value to add to the value of the signal.
 *
 */
void HSA_API
    hsa_signal_add_acq_rel(hsa_signal_t signal, hsa_signal_value_t value);

/**
 * @copydoc hsa_signal_add_acq_rel
 */
void HSA_API
    hsa_signal_add_acquire(hsa_signal_t signal, hsa_signal_value_t value);

/**
 * @copydoc hsa_signal_add_acq_rel
 */
void HSA_API
    hsa_signal_add_relaxed(hsa_signal_t signal, hsa_signal_value_t value);

/**
 * @copydoc hsa_signal_add_acq_rel
 */
void HSA_API
    hsa_signal_add_release(hsa_signal_t signal, hsa_signal_value_t value);

/**
 * @brief Atomically decrement the value of a signal by a given amount.
 *
 * @details If the value of the signal is changed, all the agents waiting on
 * @p signal for which @p value satisfies their wait condition are awakened.
 *
 * @param[in] signal Signal. If @p signal is a queue doorbell signal, the
 * behavior is undefined.
 *
 * @param[in] value Value to subtract from the value of the signal.
 *
 */
void HSA_API
    hsa_signal_subtract_acq_rel(hsa_signal_t signal, hsa_signal_value_t value);

/**
 * @copydoc hsa_signal_subtract_acq_rel
 */
void HSA_API
    hsa_signal_subtract_acquire(hsa_signal_t signal, hsa_signal_value_t value);

/**
 * @copydoc hsa_signal_subtract_acq_rel
 */
void HSA_API
    hsa_signal_subtract_relaxed(hsa_signal_t signal, hsa_signal_value_t value);

/**
 * @copydoc hsa_signal_subtract_acq_rel
 */
void HSA_API
    hsa_signal_subtract_release(hsa_signal_t signal, hsa_signal_value_t value);

/**
 * @brief Atomically perform a bitwise AND operation between the value of a
 * signal and a given value.
 *
 * @details If the value of the signal is changed, all the agents waiting on
 * @p signal for which @p value satisfies their wait condition are awakened.
 *
 * @param[in] signal Signal. If @p signal is a queue doorbell signal, the
 * behavior is undefined.
 *
 * @param[in] value Value to AND with the value of the signal.
 *
 */
void HSA_API
    hsa_signal_and_acq_rel(hsa_signal_t signal, hsa_signal_value_t value);

/**
 * @copydoc hsa_signal_and_acq_rel
 */
void HSA_API
    hsa_signal_and_acquire(hsa_signal_t signal, hsa_signal_value_t value);

/**
 * @copydoc hsa_signal_and_acq_rel
 */
void HSA_API
    hsa_signal_and_relaxed(hsa_signal_t signal, hsa_signal_value_t value);

/**
 * @copydoc hsa_signal_and_acq_rel
 */
void HSA_API
    hsa_signal_and_release(hsa_signal_t signal, hsa_signal_value_t value);

/**
 * @brief Atomically perform a bitwise OR operation between the value of a
 * signal and a given value.
 *
 * @details If the value of the signal is changed, all the agents waiting on
 * @p signal for which @p value satisfies their wait condition are awakened.
 *
 * @param[in] signal Signal. If @p signal is a queue doorbell signal, the
 * behavior is undefined.
 *
 * @param[in] value Value to OR with the value of the signal.
 */
void HSA_API
    hsa_signal_or_acq_rel(hsa_signal_t signal, hsa_signal_value_t value);

/**
 * @copydoc hsa_signal_or_acq_rel
 */
void HSA_API
    hsa_signal_or_acquire(hsa_signal_t signal, hsa_signal_value_t value);

/**
 * @copydoc hsa_signal_or_acq_rel
 */
void HSA_API
    hsa_signal_or_relaxed(hsa_signal_t signal, hsa_signal_value_t value);

/**
 * @copydoc hsa_signal_or_acq_rel
 */
void HSA_API
    hsa_signal_or_release(hsa_signal_t signal, hsa_signal_value_t value);

/**
 * @brief Atomically perform a bitwise XOR operation between the value of a
 * signal and a given value.
 *
 * @details If the value of the signal is changed, all the agents waiting on
 * @p signal for which @p value satisfies their wait condition are awakened.
 *
 * @param[in] signal Signal. If @p signal is a queue doorbell signal, the
 * behavior is undefined.
 *
 * @param[in] value Value to XOR with the value of the signal.
 *
 */
void HSA_API
    hsa_signal_xor_acq_rel(hsa_signal_t signal, hsa_signal_value_t value);

/**
 * @copydoc hsa_signal_xor_acq_rel
 */
void HSA_API
    hsa_signal_xor_acquire(hsa_signal_t signal, hsa_signal_value_t value);

/**
 * @copydoc hsa_signal_xor_acq_rel
 */
void HSA_API
    hsa_signal_xor_relaxed(hsa_signal_t signal, hsa_signal_value_t value);

/**
 * @copydoc hsa_signal_xor_acq_rel
 */
void HSA_API
    hsa_signal_xor_release(hsa_signal_t signal, hsa_signal_value_t value);

/**
 * @brief Wait condition operator.
 */
typedef enum {
  /**
   * The two operands are equal.
   */
  HSA_SIGNAL_CONDITION_EQ = 0,
  /**
   * The two operands are not equal.
   */
  HSA_SIGNAL_CONDITION_NE = 1,
  /**
   * The first operand is less than the second operand.
   */
  HSA_SIGNAL_CONDITION_LT = 2,
  /**
   * The first operand is greater than or equal to the second operand.
   */
  HSA_SIGNAL_CONDITION_GTE = 3
} hsa_signal_condition_t;

/**
 * @brief State of the application thread during a signal wait.
 */
typedef enum {
  /**
   * The application thread may be rescheduled while waiting on the signal.
   */
  HSA_WAIT_STATE_BLOCKED = 0,
  /**
   * The application thread stays active while waiting on a signal.
   */
  HSA_WAIT_STATE_ACTIVE = 1
} hsa_wait_state_t;

/**
 * @brief Wait until a signal value satisfies a specified condition, or a
 * certain amount of time has elapsed.
 *
 * @details A wait operation can spuriously resume at any time sooner than the
 * timeout (for example, due to system or other external factors) even when the
 * condition has not been met.
 *
 * The function is guaranteed to return if the signal value satisfies the
 * condition at some point in time during the wait, but the value returned to
 * the application might not satisfy the condition. The application must ensure
 * that signals are used in such way that wait wakeup conditions are not
 * invalidated before dependent threads have woken up.
 *
 * When the wait operation internally loads the value of the passed signal, it
 * uses the memory order indicated in the function name.
 *
 * @param[in] signal Signal.
 *
 * @param[in] condition Condition used to compare the signal value with @p
 * compare_value.
 *
 * @param[in] compare_value Value to compare with.
 *
 * @param[in] timeout_hint Maximum duration of the wait.  Specified in the same
 * unit as the system timestamp. The operation might block for a shorter or
 * longer time even if the condition is not met. A value of UINT64_MAX indicates
 * no maximum.
 *
 * @param[in] wait_state_hint Hint used by the application to indicate the
 * preferred waiting state. The actual waiting state is ultimately decided by
 * HSA runtime and may not match the provided hint. A value of
 * ::HSA_WAIT_STATE_ACTIVE may improve the latency of response to a signal
 * update by avoiding rescheduling overhead.
 *
 * @return Observed value of the signal, which might not satisfy the specified
 * condition.
 *
 */
hsa_signal_value_t HSA_API
    hsa_signal_wait_acquire(hsa_signal_t signal,
                            hsa_signal_condition_t condition,
                            hsa_signal_value_t compare_value,
                            uint64_t timeout_hint,
                            hsa_wait_state_t wait_state_hint);

/**
 * @copydoc hsa_signal_wait_acquire
 */
hsa_signal_value_t HSA_API
    hsa_signal_wait_relaxed(hsa_signal_t signal,
                            hsa_signal_condition_t condition,
                            hsa_signal_value_t compare_value,
                            uint64_t timeout_hint,
                            hsa_wait_state_t wait_state_hint);


/**
 * @brief Queue type. Intended to be used for dynamic queue protocol
 * determination.
 */
typedef enum {
  /**
   * Queue supports multiple producers.
   */
  HSA_QUEUE_TYPE_MULTI = 0,
  /**
   * Queue only supports a single producer.
   */
  HSA_QUEUE_TYPE_SINGLE = 1
} hsa_queue_type_t;


/**
 * @brief User mode queue.
 *
 * @details The queue structure is read-only and allocated by the HSA runtime,
 * but agents can directly modify the contents of the buffer pointed by @a
 * base_address, or use HSA runtime APIs to access the doorbell signal.
 *
 */
typedef struct hsa_queue_s {
  /**
   * Queue type.
   */
  hsa_queue_type_t type;

  /**
   * Queue features mask. This is a bit-field of ::hsa_queue_feature_t
   * values. Applications should ignore any unknown set bits.
   */
  uint32_t features;

#ifdef HSA_LARGE_MODEL
  void *base_address;
#elif defined HSA_LITTLE_ENDIAN
  /**
   * Starting address of the HSA runtime-allocated buffer used to store the AQL
   * packets. Must be aligned to the size of an AQL packet.
   */
  void *base_address;
  /**
   * Reserved. Must be 0.
   */
  uint32_t reserved0;
#else
  uint32_t reserved0;
  void *base_address;
#endif

  /**
   * Signal object used by the application to indicate the ID of a packet that
   * is ready to be processed. The HSA runtime manages the doorbell signal. If
   * the application tries to replace or destroy this signal, the behavior is
   * undefined.
   *
   * If @a type is ::HSA_QUEUE_TYPE_SINGLE the doorbell signal value must be
   * updated in a monotonically increasing fashion. If @a type is
   * ::HSA_QUEUE_TYPE_MULTI, the doorbell signal value can be updated with any
   * value.
   */
  hsa_signal_t doorbell_signal;

  /**
   * Maximum number of packets the queue can hold. Must be a power of 2.
   */
  uint32_t size;
  /**
   * Reserved. Must be 0.
   */
  uint32_t reserved1;
  /**
   * Queue identifier, which is unique over the lifetime of the application.
   */
  uint64_t id;

} hsa_queue_t;

/**
 * @brief Atomically load the read index of a queue.
 *
 * @param[in] queue Pointer to a queue.
 *
 * @return Read index of the queue pointed by @p queue.
 */
uint64_t HSA_API hsa_queue_load_read_index_acquire(const hsa_queue_t *queue);

/**
 * @copydoc hsa_queue_load_read_index_acquire
 */
uint64_t HSA_API hsa_queue_load_read_index_relaxed(const hsa_queue_t *queue);

/**
 * @brief Atomically load the write index of a queue.
 *
 * @param[in] queue Pointer to a queue.
 *
 * @return Write index of the queue pointed by @p queue.
 */
uint64_t HSA_API hsa_queue_load_write_index_acquire(const hsa_queue_t *queue);

/**
 * @copydoc hsa_queue_load_write_index_acquire
 */
uint64_t HSA_API hsa_queue_load_write_index_relaxed(const hsa_queue_t *queue);

/**
 * @brief Atomically set the write index of a queue.
 *
 * @param[in] queue Pointer to a queue.
 *
 * @param[in] value Value to assign to the write index.
 *
 */
void HSA_API hsa_queue_store_write_index_relaxed(const hsa_queue_t *queue,
                                                 uint64_t value);

/**
 * @copydoc hsa_queue_store_write_index_relaxed
 */
void HSA_API hsa_queue_store_write_index_release(const hsa_queue_t *queue,
                                                 uint64_t value);

/**
 * @brief Atomically set the write index of a queue if the observed value is
 * equal to the expected value. The application can inspect the returned value
 * to determine if the replacement was done.
 *
 * @param[in] queue Pointer to a queue.
 *
 * @param[in] expected Expected value.
 *
 * @param[in] value Value to assign to the write index if @p expected matches
 * the observed write index. Must be greater than @p expected.
 *
 * @return Previous value of the write index.
 */
uint64_t HSA_API hsa_queue_cas_write_index_acq_rel(const hsa_queue_t *queue,
                                                   uint64_t expected,
                                                   uint64_t value);

/**
 * @copydoc hsa_queue_cas_write_index_acq_rel
 */
uint64_t HSA_API hsa_queue_cas_write_index_acquire(const hsa_queue_t *queue,
                                                   uint64_t expected,
                                                   uint64_t value);

/**
 * @copydoc hsa_queue_cas_write_index_acq_rel
 */
uint64_t HSA_API hsa_queue_cas_write_index_relaxed(const hsa_queue_t *queue,
                                                   uint64_t expected,
                                                   uint64_t value);

/**
 * @copydoc hsa_queue_cas_write_index_acq_rel
 */
uint64_t HSA_API hsa_queue_cas_write_index_release(const hsa_queue_t *queue,
                                                   uint64_t expected,
                                                   uint64_t value);

/**
 * @brief Atomically increment the write index of a queue by an offset.
 *
 * @param[in] queue Pointer to a queue.
 *
 * @param[in] value Value to add to the write index.
 *
 * @return Previous value of the write index.
 */
uint64_t HSA_API
    hsa_queue_add_write_index_acq_rel(const hsa_queue_t *queue, uint64_t value);

/**
 * @copydoc hsa_queue_add_write_index_acq_rel
 */
uint64_t HSA_API
    hsa_queue_add_write_index_acquire(const hsa_queue_t *queue, uint64_t value);

/**
 * @copydoc hsa_queue_add_write_index_acq_rel
 */
uint64_t HSA_API
    hsa_queue_add_write_index_relaxed(const hsa_queue_t *queue, uint64_t value);

/**
 * @copydoc hsa_queue_add_write_index_acq_rel
 */
uint64_t HSA_API
    hsa_queue_add_write_index_release(const hsa_queue_t *queue, uint64_t value);

/**
 * @brief Atomically set the read index of a queue.
 *
 * @details Modifications of the read index are not allowed and result in
 * undefined behavior if the queue is associated with an agent for which
 * only the corresponding packet processor is permitted to update the read
 * index.
 *
 * @param[in] queue Pointer to a queue.
 *
 * @param[in] value Value to assign to the read index.
 *
 */
void HSA_API hsa_queue_store_read_index_relaxed(const hsa_queue_t *queue,
                                                uint64_t value);

/**
 * @copydoc hsa_queue_store_read_index_relaxed
 */
void HSA_API hsa_queue_store_read_index_release(const hsa_queue_t *queue,
                                                uint64_t value);
/** @} */

/** \defgroup aql Architected Queuing Language
 *  @{
 */

/**
 * @brief Packet type.
 */
typedef enum {
  /**
   * Vendor-specific packet.
   */
  HSA_PACKET_TYPE_VENDOR_SPECIFIC = 0,
  /**
   * The packet has been processed in the past, but has not been reassigned to
   * the packet processor. A packet processor must not process a packet of this
   * type. All queues support this packet type.
   */
  HSA_PACKET_TYPE_INVALID = 1,
  /**
   * Packet used by agents for dispatching jobs to kernel agents. Not all
   * queues support packets of this type (see ::hsa_queue_feature_t).
   */
  HSA_PACKET_TYPE_KERNEL_DISPATCH = 2,
  /**
   * Packet used by agents to delay processing of subsequent packets, and to
   * express complex dependencies between multiple packets. All queues support
   * this packet type.
   */
  HSA_PACKET_TYPE_BARRIER_AND = 3,
  /**
   * Packet used by agents for dispatching jobs to agents.  Not all
   * queues support packets of this type (see ::hsa_queue_feature_t).
   */
  HSA_PACKET_TYPE_AGENT_DISPATCH = 4,
  /**
   * Packet used by agents to delay processing of subsequent packets, and to
   * express complex dependencies between multiple packets. All queues support
   * this packet type.
   */
  HSA_PACKET_TYPE_BARRIER_OR = 5
} hsa_packet_type_t;

/**
 * @brief Scope of the memory fence operation associated with a packet.
 */
typedef enum {
  /**
   * No scope (no fence is applied). The packet relies on external fences to
   * ensure visibility of memory updates.
   */
  HSA_FENCE_SCOPE_NONE = 0,
  /**
   * The fence is applied with agent scope for the global segment.
   */
  HSA_FENCE_SCOPE_AGENT = 1,
  /**
   * The fence is applied across both agent and system scope for the global
   * segment.
   */
  HSA_FENCE_SCOPE_SYSTEM = 2
} hsa_fence_scope_t;

/**
 * @brief Sub-fields of the @a header field that is present in any AQL
 * packet. The offset (with respect to the address of @a header) of a sub-field
 * is identical to its enumeration constant. The width of each sub-field is
 * determined by the corresponding value in ::hsa_packet_header_width_t. The
 * offset and the width are expressed in bits.
 */
typedef enum {
  /**
   * Packet type. The value of this sub-field must be one of
   * ::hsa_packet_type_t. If the type is ::HSA_PACKET_TYPE_VENDOR_SPECIFIC, the
   * packet layout is vendor-specific.
   */
  HSA_PACKET_HEADER_TYPE = 0,
  /**
   * Barrier bit. If the barrier bit is set, the processing of the current
   * packet only launches when all preceding packets (within the same queue) are
   * complete.
   */
  HSA_PACKET_HEADER_BARRIER = 8,
  /**
   * Acquire fence scope. The value of this sub-field determines the scope and
   * type of the memory fence operation applied before the packet enters the
   * active phase. An acquire fence ensures that any subsequent global segment
   * or image loads by any unit of execution that belongs to a dispatch that has
   * not yet entered the active phase on any queue of the same kernel agent,
   * sees any data previously released at the scopes specified by the acquire
   * fence. The value of this sub-field must be one of ::hsa_fence_scope_t.
   */
  HSA_PACKET_HEADER_ACQUIRE_FENCE_SCOPE = 9,
  /**
   * Release fence scope, The value of this sub-field determines the scope and
   * type of the memory fence operation applied after kernel completion but
   * before the packet is completed. A release fence makes any global segment or
   * image data that was stored by any unit of execution that belonged to a
   * dispatch that has completed the active phase on any queue of the same
   * kernel agent visible in all the scopes specified by the release fence. The
   * value of this sub-field must be one of ::hsa_fence_scope_t.
   */
  HSA_PACKET_HEADER_RELEASE_FENCE_SCOPE = 11
} hsa_packet_header_t;

/**
 * @brief Width (in bits) of the sub-fields in ::hsa_packet_header_t.
 */
typedef enum {
  HSA_PACKET_HEADER_WIDTH_TYPE = 8,
  HSA_PACKET_HEADER_WIDTH_BARRIER = 1,
  HSA_PACKET_HEADER_WIDTH_ACQUIRE_FENCE_SCOPE = 2,
  HSA_PACKET_HEADER_WIDTH_RELEASE_FENCE_SCOPE = 2
} hsa_packet_header_width_t;

/**
 * @brief Sub-fields of the kernel dispatch packet @a setup field. The offset
 * (with respect to the address of @a setup) of a sub-field is identical to its
 * enumeration constant. The width of each sub-field is determined by the
 * corresponding value in ::hsa_kernel_dispatch_packet_setup_width_t. The
 * offset and the width are expressed in bits.
 */
typedef enum {
  /**
   * Number of dimensions of the grid. Valid values are 1, 2, or 3.
   *
   */
  HSA_KERNEL_DISPATCH_PACKET_SETUP_DIMENSIONS = 0
} hsa_kernel_dispatch_packet_setup_t;

/**
 * @brief Width (in bits) of the sub-fields in
 * ::hsa_kernel_dispatch_packet_setup_t.
 */
typedef enum {
  HSA_KERNEL_DISPATCH_PACKET_SETUP_WIDTH_DIMENSIONS = 2
} hsa_kernel_dispatch_packet_setup_width_t;

/**
 * @brief AQL kernel dispatch packet
 */
typedef struct hsa_kernel_dispatch_packet_s {
  /**
   * Packet header. Used to configure multiple packet parameters such as the
   * packet type. The parameters are described by ::hsa_packet_header_t.
   */
  uint16_t header;

  /**
   * Dispatch setup parameters. Used to configure kernel dispatch parameters
   * such as the number of dimensions in the grid. The parameters are described
   * by ::hsa_kernel_dispatch_packet_setup_t.
   */
  uint16_t setup;

  /**
   * X dimension of work-group, in work-items. Must be greater than 0.
   */
  uint16_t workgroup_size_x;

  /**
   * Y dimension of work-group, in work-items. Must be greater than
   * 0. If the grid has 1 dimension, the only valid value is 1.
   */
  uint16_t workgroup_size_y;

  /**
   * Z dimension of work-group, in work-items. Must be greater than
   * 0. If the grid has 1 or 2 dimensions, the only valid value is 1.
   */
  uint16_t workgroup_size_z;

  /**
   * Reserved. Must be 0.
   */
  uint16_t reserved0;

  /**
   * X dimension of grid, in work-items. Must be greater than 0. Must
   * not be smaller than @a workgroup_size_x.
   */
  uint32_t grid_size_x;

  /**
   * Y dimension of grid, in work-items. Must be greater than 0. If the grid has
   * 1 dimension, the only valid value is 1. Must not be smaller than @a
   * workgroup_size_y.
   */
  uint32_t grid_size_y;

  /**
   * Z dimension of grid, in work-items. Must be greater than 0. If the grid has
   * 1 or 2 dimensions, the only valid value is 1. Must not be smaller than @a
   * workgroup_size_z.
   */
  uint32_t grid_size_z;

  /**
   * Size in bytes of private memory allocation request (per work-item).
   */
  uint32_t private_segment_size;

  /**
   * Size in bytes of group memory allocation request (per work-group). Must not
   * be less than the sum of the group memory used by the kernel (and the
   * functions it calls directly or indirectly) and the dynamically allocated
   * group segment variables.
   */
  uint32_t group_segment_size;

  /**
   * Opaque handle to a code object that includes an implementation-defined
   * executable code for the kernel.
   */
  uint64_t kernel_object;

#ifdef HSA_LARGE_MODEL
  void *kernarg_address;
#elif defined HSA_LITTLE_ENDIAN
  /**
   * Pointer to a buffer containing the kernel arguments. May be NULL.
   *
   * The buffer must be allocated using ::hsa_memory_allocate, and must not be
   * modified once the kernel dispatch packet is enqueued until the dispatch has
   * completed execution.
   */
  void *kernarg_address;
  /**
   * Reserved. Must be 0.
   */
  uint32_t reserved1;
#else
  uint32_t reserved1;
  void *kernarg_address;
#endif

  /**
   * Reserved. Must be 0.
   */
  uint64_t reserved2;

  /**
   * Signal used to indicate completion of the job. The application can use the
   * special signal handle 0 to indicate that no signal is used.
   */
  hsa_signal_t completion_signal;

} hsa_kernel_dispatch_packet_t;

/**
 * @brief Agent dispatch packet.
 */
typedef struct hsa_agent_dispatch_packet_s {
  /**
   * Packet header. Used to configure multiple packet parameters such as the
   * packet type. The parameters are described by ::hsa_packet_header_t.
   */
  uint16_t header;

  /**
   * Application-defined function to be performed by the destination agent.
   */
  uint16_t type;

  /**
   * Reserved. Must be 0.
   */
  uint32_t reserved0;

#ifdef HSA_LARGE_MODEL
  void *return_address;
#elif defined HSA_LITTLE_ENDIAN
  /**
   * Address where to store the function return values, if any.
   */
  void *return_address;
  /**
   * Reserved. Must be 0.
   */
  uint32_t reserved1;
#else
  uint32_t reserved1;
  void *return_address;
#endif

  /**
   * Function arguments.
   */
  uint64_t arg[4];

  /**
   * Reserved. Must be 0.
   */
  uint64_t reserved2;

  /**
   * Signal used to indicate completion of the job. The application can use the
   * special signal handle 0 to indicate that no signal is used.
   */
  hsa_signal_t completion_signal;

} hsa_agent_dispatch_packet_t;

/**
 * @brief Barrier-AND packet.
 */
typedef struct hsa_barrier_and_packet_s {
  /**
   * Packet header. Used to configure multiple packet parameters such as the
   * packet type. The parameters are described by ::hsa_packet_header_t.
   */
  uint16_t header;

  /**
   * Reserved. Must be 0.
   */
  uint16_t reserved0;

  /**
   * Reserved. Must be 0.
   */
  uint32_t reserved1;

  /**
   * Array of dependent signal objects. Signals with a handle value of 0 are
   * allowed and are interpreted by the packet processor as satisfied
   * dependencies.
   */
  hsa_signal_t dep_signal[5];

  /**
   * Reserved. Must be 0.
   */
  uint64_t reserved2;

  /**
   * Signal used to indicate completion of the job. The application can use the
   * special signal handle 0 to indicate that no signal is used.
   */
  hsa_signal_t completion_signal;

} hsa_barrier_and_packet_t;

/**
 * @brief Barrier-OR packet.
 */
typedef struct hsa_barrier_or_packet_s {
  /**
   * Packet header. Used to configure multiple packet parameters such as the
   * packet type. The parameters are described by ::hsa_packet_header_t.
   */
  uint16_t header;

  /**
   * Reserved. Must be 0.
   */
  uint16_t reserved0;

  /**
   * Reserved. Must be 0.
   */
  uint32_t reserved1;

  /**
   * Array of dependent signal objects. Signals with a handle value of 0 are
   * allowed and are interpreted by the packet processor as dependencies not
   * satisfied.
   */
  hsa_signal_t dep_signal[5];

  /**
   * Reserved. Must be 0.
   */
  uint64_t reserved2;

  /**
   * Signal used to indicate completion of the job. The application can use the
   * special signal handle 0 to indicate that no signal is used.
   */
  hsa_signal_t completion_signal;

} hsa_barrier_or_packet_t;

/* System-wide atomic ops for 32bit */
int hsa_atomic_load_system(int *p);
int hsa_atomic_and_system(int *p, int val);
int hsa_atomic_or_system(int *p, int val);
int hsa_atomic_xor_system(int *p, int val);
int hsa_atomic_xchg_system(int *p, int val);
int hsa_atomic_add_system(int *p, int val);
int hsa_atomic_sub_system(int *p, int val);
int hsa_atomic_max_system(int *p, int val);
int hsa_atomic_min_system(int *p, int val);
int hsa_atomic_cas_system(int *p, int val1, int val2);


/* System-wide atomic ops for 64bit */
long hsa_atomic_load_system_long(long *p);
long hsa_atomic_and_system_long(long *p, long val);
long hsa_atomic_or_system_long(long *p, long val);
long hsa_atomic_xor_system_long(long *p, long val);
long hsa_atomic_xchg_system_long(long *p, long val);
long hsa_atomic_add_system_long(long *p, long val);
long hsa_atomic_sub_system_long(long *p, long val);
long hsa_atomic_max_system_long(long *p, long val);
long hsa_atomic_min_system_long(long *p, long val);
long hsa_atomic_cas_system_long(long *p, long val1, long val2);


#ifdef __cplusplus
}  // end extern "C" block
#endif

#endif  // header guard
