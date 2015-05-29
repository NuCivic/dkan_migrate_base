<?php

/**
 * This file provides a class that enables superclasses to call the import of
 * resources from CKAN.
 */

/**
 * Class MigrateCkanResources
 */
abstract class MigrateCkanResourcesBase extends MigrateCkanDatasetBase {

  private $machine_name = '';

  public function __construct($arguments) {
    parent::__construct($arguments);

    $this->machine_name = $arguments['machine_name'];
  }

  /**
   * Overload the prepareRow function to import the resources. Then run the
   * standard import.
   *
   * @param object $row
   */
  public function prepareRow($row) {
    // Migrate the resources for this dataset.
    $this->migrateResources($row->resources);

    // This method should return TRUE/FALSE (see http://www.drupalcontrib.org/api/drupal/contributions!migrate!includes!migration.inc/function/Migration::prepareRow/7)
    return parent::prepareRow($row);
  }

  /**
   * Migrate the specified resources.
   *
   * @param array $resources An array of resources ids.
   */
  protected function migrateResources($resources) {

    // Return if there are no resources.
    if (!is_array($resources)) {
      return;
    }

    drupal_set_message(t('Migrating %count resource(s)', array('%count' => sizeof($resources))));

    // Create a machine name by combining the dataset machine name with '_resources',
    // then create a file with that machine name, that will include a JSON representation
    // of the resources related to a dataset (provided as an argument to this function.
    $machine_name = $this->machine_name . '_resources';
    $filename     = "public://$machine_name";

    file_unmanaged_save_data(json_encode($resources), $filename, FILE_EXISTS_REPLACE);

    // Register the migration, then get its instance and start it.
    MigrationBase::registerMigration('MigrateCkanResources', $machine_name, array('resources' => $filename));

    // Migrate has a static reference to the current migration that will be overwritten
    // by the following import process, therefore we save the current reference
    // and restore it when process is complete.
    $current_migration = self::$currentMigration;

    $migration = Migration::getInstance($machine_name);
    $result    = $migration->processImport();

    self::$currentMigration = $current_migration;

    // Delete the file with the list of resources and remove the migration.
    file_unmanaged_delete($filename);
    MigrationBase::deregisterMigration($machine_name);

    // Return the result from the processImport call.
    return $result;
  }

}

class DkanMigrateList extends MigrateList {

  private $items;

  public function __construct($items) {
    parent::__construct();

    $this->items = $items;
  }

  /**
   * Implementors are expected to return a string representing where the listing
   * is obtained from (a URL, file directory, etc.)
   *
   * @return string
   */
  public function __toString() {
    // TODO: Implement __toString() method.
    return 'TODO: ...';
  }

  /**
   * Implementors are expected to return an array of unique IDs, suitable for
   * passing to the MigrateItem class to retrieve the data for a single item.
   *
   * @return Mixed, iterator or array
   */
  public function getIdList() {

    return array_keys($this->items);
  }

  /**
   * Implementors are expected to return a count of IDs available to be migrated.
   *
   * @return int
   */
  public function computeCount() {

    return (is_array($this->items) ? sizeof($this->items) : -1);
  }

  /**
   * Implementors are expected to return a count of IDs available to be migrated.
   *
   * @return int
   */
  public function count() {

    return $this->computeCount();
  }
}

class DkanMigrateItem extends MigrateItem {

  private $items;

  public function __construct($items = array()) {
    parent::__construct();

    $this->$items = $items;
  }

  /**
   * Implementors are expected to return an object representing a source item.
   *
   * @param mixed $id
   *
   * @return stdClass
   */
  public function getItem($id) {

    return $this->items[$id];
  }
}

/**
 *
 */
class MigrateCkanResources extends MigrateCkanResourceBase {

  public function __construct($arguments) {
    parent::__construct($arguments);

    // Check that the required resources argument is provided.
    if (!isset($arguments['resources'])) {
      return;
    }

    // Allow superclass to override the list of fields.
    $fields = $this->getCkanResourceFields();

    // Get the filename of the local file containing the JSON representation of
    // the resources.
    $filename = drupal_realpath($arguments['resources']);
    if (empty($filename)) {
      drupal_set_message('This migration needs a filename', 'warn');
      return;
    }

    // The source is set in the super-class, we now overwrite it using our own
    // implementation.
    $this->source = new MigrateSourceList(
      new DkanMigrateListJSON($filename),
      new DkanMigrateItemJSON($filename, array()),
      $fields
    );

  }
}


class DkanMigrateListJSON extends MigrateListJSON {
  /**
   * The default implementation assumes the IDs are top-level array elements,
   * but the array elements are the data items - we need to look inside them
   * for the IDs.
   */
  protected function getIDsFromJSON(array $data) {
    $ids = array();
    foreach ($data as $item) {
      $ids[] = $item['id'];
    }
    return $ids;
  }
}

class DkanMigrateItemJSON extends MigrateItemJSON {
  protected $data = array();

  /**
   * Two problems with the default getItem() - it reads and parses the JSON on
   * each call, which we don't want to do for multiple items in the file; and,
   * we need to retrieve a given item from the file according to its 'id' element.
   */
  public function getItem($id) {
    // We cache the parsed JSON at $this->data.
    if (empty($this->data)) {
      $data = $this->loadJSONUrl($this->itemUrl);
      if ($data) {
        // Let's index the array by the ID for easy retrieval.
        foreach ($data as $item) {
          $this->data[$item->id] = $item;
        }
      }
      else {
        // Error-handling here....
      }
    }
    // Return the requested item
    if (isset($this->data[$id])) {
      return $this->data[$id];
    }
    else {
      return NULL;
    }
  }
}
